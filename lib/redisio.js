/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019 Streampunk Media Ltd.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  https://www.streampunk.media/ mailto:furnace@streampunk.media
  14 Ormiscaig, Aultbea, Achnasheen, IV22 2JJ  U.K.
*/

const Redis = require('ioredis');
const mappings = require('./mappings.js');
const config = require('../config.json');
const uuid = require('uuid');
const beamcoder = require('beamcoder');

/*
   Basic idea ...

   format stored at 'beamcoder:<url>' - TODO how to deal with multiple formats
   list of formats at beamcoder:content
   streams for format stored at 'beamcoder:<url>:stream_<index>'
     (streams embed codec parameters and contain nb_streams counter)
   packets for stream stored at 'beamcoder:<url>:stream_<index>:packet_<pts>'
   frames for stream stored at 'beamcoder:<url>:stream_<index>:frame_<pts>'
   sorted index sorted set is stored at 'beamcoder:<url>:stream_<index>:index'

   If <pts> is not available, a counter will be used.
   If <url> is empty, a UUID will be generated and stored as a urn.
   'beamcoder' is taken from config file
   Index maps pts to keys of frames or packets
*/

const EMPTY_SLOT = Object.freeze(Object.create(null));

const zip = (...rows) => [...rows[0]].map((_,c) => rows.map(row => row[c]));

const redisPool = {
  pool: [],
  nextFree: null,
  queue: [],
  closeCheck: null,
  closeResolve: null,
  testing: false,
  closeTimer: null,
  use: async function() {
    if (this.nextFree == null || this.nextFree == this.pool.length) {
      if (!this.grow( this.pool.length || 5 )) {
        let wait = new Promise((fulfil) => {
          this.queue.push(fulfil);
        });
        return wait;
      }
    }
    let obj = this.pool[this.nextFree];
    this.pool[this.nextFree++] = EMPTY_SLOT;
    return obj;
  },
  recycle: function (obj) {
    if (this.nextFree === null || this.nextFree === -1) {
      this.pool[this.pool.length] = obj;
    } else {
      this.pool[--this.nextFree] = obj;
    }
    if (this.queue.length > 0) {
      this.queue.shift()(this.use());
    }
    if (this.closeResolve) {
      this.close();
    }
  },
  grow: function (count = this.pool.length) {
    if (count > 0 && this.nextFree == null) {
      this.nextFree = 0;
    }

    if ((this.pool.length + count) > config.redis.pool) {
      return false;
    }

    if (count > 0) {
      let curLen = this.pool.length;
      this.pool.length += count;
      for ( let x = curLen ; x < this.pool.length ; x++ ) {
        // console.log(x, this.testing, config.redis.db, config.testing.db);
        this.pool[x] = new Redis({
          port: config.redis.port,
          host: config.redis.host,
          db: this.testing ? config.testing.db : config.redis.db
        });
      }
    }
    return true;
  },
  size: function () {
    return this.pool.length;
  },
  close: async function () {
    if (this.pool.every(x => x !== EMPTY_SLOT)) {
      let result = Promise.all(
        this.pool.filter(x => x !== EMPTY_SLOT)
          .map(x => x.quit()));
      this.queue.forEach(x => x(null));
      this.queue = [];
      this.pool = [];
      this.nextFree = null;
      this.closeCheck = null;
      if (this.closeResolve) this.closeResolve();
      this.closeResolve = null;
      if (this.closeTimer) clearTimeout(this.closeTimer);
      this.closeTimer = null;
      return result;
    } else {
      this.closeCheck = this.closeCheck ? this.closeCheck : new Promise((f, r) => {
        this.closeResolve = f;
        this.closeTimer = setTimeout(() => {
          r(`Failed to close all redis connections within ${config.redis.closeTimeout}ms.`);
          this.closeCheck = null;
          this.closeResolve = null;
        }, config.redis.closeTimeout);
      });
      return this.closeCheck;
    }
  }
};

['pool', 'nextFree', 'queue', 'grow', 'closeCheck', 'closeResolve', 'closeTimer'].forEach(p => {
  Object.defineProperty(redisPool, p, {
    enumerable: false
  });
});

async function listContent(start = 0, limit = 10) {
  let redis = await redisPool.use();
  let list = await redis.lrange(`${config.redis.prepend}:index`, start,
    (start < 0 && start + limit < limit) ? -1 : start + limit - 1);
  redisPool.recycle(redis);
  return list.map(x => x.slice(config.redis.prepend.length + 1));
}

async function storeFormat(fmt, overwrite = true) {
  let redis = await Promise.all(Array.from(
    new Array(fmt.streams.length + 1), () => redisPool.use()));
  if ((!fmt.url) || (fmt.url && fmt.url.length === 0)) {
    fmt.url = `urn:uuid:${uuid.v4()}`;
  }
  let key = `${config.redis.prepend}:${fmt.url}`;
  if (!overwrite) {
    let exists = await redis[0].exists(key);
    if (exists) throw new Error(`A format with key '${key}' already exists.`);
  }
  let delWork = [ redis[0].del(key) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    delWork.push(redis[x].del(`${key}:stream_${x-1}`));
  }
  await Promise.all(delWork);
  let setWork = [ redis[0].hmset(key, mappings.formatToRedis(fmt.toJSON())) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    setWork.push(
      redis[x].hmset(`${key}:stream_${x-1}`,
        mappings.streamToRedis(fmt.streams[x-1].toJSON())));
  }
  let result = await Promise.all(setWork);
  await redis[0].rpush(`${config.redis.prepend}:index`, key);

  redis.forEach(x => redisPool.recycle(x));
  return result;
}

// TODO wot no pts?

async function storePacket(fmt, packet) {
  let redis = await Promise.all([redisPool.use(), redisPool.use()]);
  let idxk = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${packet.stream_index}:index`;
  let key = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${packet.stream_index}:packet_${packet.pts}`;
  let datak = `${key}:data`;
  await Promise.all([redis[0].del(key), redis[1].del(datak)]);
  let result = await Promise.all([
    redis[0].hmset(key, mappings.packetToRedis(packet.toJSON())),
    redis[1].setBuffer(datak, packet.data) ]);
  await Promise.all([
    redis[0].zadd(idxk, packet.pts, key),
    redis[1].pexpire(datak, config.redis.packetTTL) ]);
  redis.forEach(x => redisPool.recycle(x));
  return result.map(x => Buffer.isBuffer(x) ? x.toString() : x);
}

async function storeFrame(fmt, frame, stream_index) {
  let redis = await Promise.all(
    Array.from(new Array(frame.data.length + 1), () => redisPool.use()));
  if (!stream_index) {
    stream_index = frame.stream_index;
    if (!stream_index) {
      throw new Error('Cannot determine stream_index for frame.');
    }
  }
  frame.stream_index = stream_index;
  let idxk = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}:index`;
  let key = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}:frame_${frame.pts}`;
  let datak_base = `${key}:data`;

  let delWork = [ redis[0].del(key) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    delWork.push(redis[x].del(`${datak_base}_${x-1}`));
  }
  await Promise.all(delWork);

  let setWork = [ redis[0].hmset(key, mappings.frameToRedis(frame.toJSON())) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    setWork.push(redis[x].setBuffer(`${datak_base}_${x-1}`, frame.data[x-1]));
  }
  let result1 = (await Promise.all(setWork))
    .map(x => Buffer.isBuffer(x) ? x.toString() : x);
  let indexWork = [ redis[0].zadd(idxk, frame.pts, key) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    indexWork.push(redis[x].pexpire(`${datak_base}_${x-1}`, config.redis.frameTTL));
  }
  let result2 = await Promise.all(indexWork);
  redis.forEach(x => redisPool.recycle(x));
  return zip(result1, result2).map(x => x[0] === 'OK' && x[1] === 1 ? 'OK' : null );
}

async function storeMedia(fmt, element, stream_index) {
  switch (element.type) {
  case 'Packet':
    return storePacket(fmt, element);
  case 'Frame':
    return storeFrame(fmt, element, stream_index);
  default:
    throw new Error('Cannot store media element unless of type Packet or Frame.');
  }
}

async function retrieveFormat(url) {
  let redisZero = await redisPool.use();
  let key = `${config.redis.prepend}:${url}`;
  let fmtb = await redisZero.hgetallBuffer(key);
  if ((!fmtb) || (Object.keys(fmtb).length === 0)) {
    redisPool.recycle(redisZero);
    throw new Error(`Unable to retrieve a format with key '${key}'.`);
  }
  let fmto = mappings.formatFromRedis(fmtb);
  let result = beamcoder.format(fmto);
  if (fmto.nb_streams > 0) {
    let redisProms = [ redisZero ];
    for ( let x = 1 ; x < fmto.nb_streams ; x++ ) {
      redisProms.push(redisPool.use());
    }
    let redis = await Promise.all(redisProms);
    let getWork = [];
    for ( let x = 0 ; x < redis.length ; x++ ) {
      getWork.push(redis[x].hgetallBuffer(`${key}:stream_${x}`));
    }
    let strs = await Promise.all(getWork);
    if (strs.some(x => x === null)) {
      redis.forEach(x => redisPool.recycle(x));
      throw new Error(`Cannot retrieve at least one of the streams '${strs.map(x => x ? '.' : 'X')}'.`);
    }
    for ( let x = 0 ; x < strs.length ; x++ ) {
      result.newStream(mappings.streamFromRedis(strs[x]));
    }
    redis.forEach(x => redisPool.recycle(x));
  } else {
    redisPool.recycle(redisZero);
  }
  return result;
}

async function retrieveStream(url, stream_index) {
  let redis = await redisPool.use();
  let key = `${config.redis.prepend}:${url}:stream_${stream_index}`;
  let strb = await redis.hgetallBuffer(key);
  if (!strb) {
    throw new Error(`Unable to retrieve a streams with key '${key}'.`);
  }
  redisPool.recycle(redis);
  let stro = mappings.streamFromRedis(strb);
  let fmt = beamcoder.format();
  let result = fmt.newStream(stro); // FIX_ME stream_index will be wrong
  return result;
}

async function retrieveMedia(fmt, stream_index, pts_start,
  pts_end = Number.MAX_SAFE_INTEGER, offset = 0, limit = 10) {
  if (arguments.length == 5) {
    [pts_end, offset, limit] = [Number.MAX_SAFE_INTEGER, pts_end, offset];
  }
  let redisZero = await redisPool.use();
  let basek =  `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}`;
  let idxk = `${basek}:index`;
  let mediaElements =
    await redisZero.zrangebyscore(idxk, pts_start, pts_end, 'limit', offset, limit);
  redisPool.recycle(redisZero);
  return await Promise.all(mediaElements.map(k =>
    k.indexOf('packet') >= 0 ? retrievePacket(k) : retrieveFrame(k) ));
}

async function retrieveMediaMetadata(fmt, stream_index, pts_start,
  pts_end = Number.MAX_SAFE_INTEGER, offset = 0, limit = 10) {
  if (arguments.length == 5) {
    [pts_end, offset, limit] = [Number.MAX_SAFE_INTEGER, pts_end, offset];
  }
  let redisZero = await redisPool.use();
  let basek =  `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}`;
  let idxk = `${basek}:index`;
  let mediaElements =
    await redisZero.zrangebyscore(idxk, pts_start, pts_end, 'limit', offset, limit);
  redisPool.recycle(redisZero);
  return await Promise.all(mediaElements.map(k =>
    k.indexOf('packet') >= 0 ? retrievePacketMetadata(k) : retrieveFrameMetadata(k) ));
}

async function retrievePacket(fmtOrKey, stream_index = 0, pts = 0) {
  let redis = await Promise.all([ redisPool.use(), redisPool.use() ]);
  let key = (typeof fmtOrKey === 'string') ?
    (fmtOrKey.startsWith(config.redis.prepend) ? fmtOrKey :
      `${config.redis.prepend}:${fmtOrKey}:stream_${stream_index}:packet_${pts}`) :
    `${config.redis.prepend}:${fmtOrKey.url}:stream_${stream_index}:packet_${pts}`;
  let dbreq = await Promise.all([
    redis[0].hgetallBuffer(key),
    redis[1].getBuffer(`${key}:data`) ]);
  if (!dbreq[0]) {
    throw new Error(`Unable to retrieve packet with key ${key}.`);
  }
  let pkt = beamcoder.packet(mappings.packetFromRedis(dbreq[0]));
  if (dbreq[1]) {
    pkt.data = dbreq[1];
  }
  redisPool.recycle(redis[0]);
  redisPool.recycle(redis[1]);
  return pkt;
}

async function retrievePacketMetadata(fmtOrKey, stream_index = 0, pts = 0) {
  let redis = await redisPool.use();
  let key = (typeof fmtOrKey === 'string') ?
    (fmtOrKey.startsWith(config.redis.prepend) ? fmtOrKey :
      `${config.redis.prepend}:${fmtOrKey}:stream_${stream_index}:packet_${pts}`) :
    `${config.redis.prepend}:${fmtOrKey.url}:stream_${stream_index}:packet_${pts}`;
  let pktb = await redis.hgetallBuffer(key);
  let pkt = beamcoder.packet(mappings.packetFromRedis(pktb));
  redisPool.recycle(redis);
  return pkt;
}

async function retrieveFrame(fmtOrKey, stream_index = 0, pts = 0) {
  let redisZero = await redisPool.use();
  let key = (typeof fmtOrKey === 'string') ?
    (fmtOrKey.startsWith(config.redis.prepend) ? fmtOrKey :
      `${config.redis.prepend}:${fmtOrKey}:stream_${stream_index}:frame_${pts}`) :
    `${config.redis.prepend}:${fmtOrKey.url}:stream_${stream_index}:frame_${pts}`;
  let frameb = await redisZero.hgetallBuffer(key);
  let frm = beamcoder.frame(mappings.frameFromRedis(frameb));
  if (frm.buf_sizes.length > 0) {
    let redis = [ redisZero ];
    for ( let x = 1 ; x < frm.buf_sizes.length ; x++ ) {
      redis.push(await redisPool.use());
    }
    let getData = [];
    for ( let x = 0 ; x < redis.length ; x++ ) {
      getData.push(redis[x].getBuffer(`${key}:data_${x}`));
    }
    let data = await Promise.all(getData);
    redis.forEach(x => redisPool.recycle(x));
    if (data.every(x => x !== null)) {
      frm.data = data;
    } else if (data.every(x => x === null)) {
      frm.data = [];
    }
    else { // TODO this will probably cauise an f-up at some point
      throw new Error(`Unable to retrieve data for some of the planes for key '${key}':` +
        `${data.map(x => x ? 'o' : 'X')}`);
    } // otherwise no data was stored / retrieved
  } else {
    redisPool.recycle(redisZero);
  }
  frm.stream_index = stream_index;
  return frm;
}

async function retrieveFrameMetadata(fmtOrKey, stream_index = 0, pts = 0) {
  let redis = await redisPool.use();
  let key = (typeof fmtOrKey === 'string') ?
    (fmtOrKey.startsWith(config.redis.prepend) ? fmtOrKey :
      `${config.redis.prepend}:${fmtOrKey}:stream_${stream_index}:frame_${pts}`) :
    `${config.redis.prepend}:${fmtOrKey.url}:stream_${stream_index}:frame_${pts}`;
  let frameb = await redis.hgetallBuffer(key);
  let frm = beamcoder.frame(mappings.frameFromRedis(frameb));
  redisPool.recycle(redis);
  frm.stream_index = stream_index;
  return frm;
}

module.exports = {
  redisPool,
  listContent,
  storeFormat,
  storeMedia,
  retrieveFormat,
  retrieveStream,
  retrieveMedia,
  retrievePacket,
  retrieveFrame,
  retrieveMediaMetadata,
  retrievePacketMetadata,
  retrieveFrameMetadata,
  close: async () => { return redisPool.close(); }
};

// TODO Delete operations.