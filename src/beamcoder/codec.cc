/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019  Streampunk Media Ltd.

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

#include "codec.h"

napi_value getCodecCtxCodecID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->codec_id, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, codec->codec->name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxLongName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, codec->codec->long_name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxCodecTag(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;
  char codecTag[AV_FOURCC_MAX_STRING_SIZE];

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  av_fourcc_make_string(codecTag, codec->codec_tag);
  status = napi_create_string_utf8(env, codecTag, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxPrivData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;
  int64_t iValue;
  double dValue;
  uint8_t* data;
  AVRational qValue;
  int ret;
  const AVOption* option = nullptr;
  const AVOption* prev = nullptr;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;

  if ((codec->codec->priv_class == nullptr) || (codec->priv_data == nullptr)) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
    return result;
  }

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  status = beam_set_string_utf8(env, result, "type", (char*) codec->codec->priv_class->class_name);
  CHECK_STATUS;
  while ((option = av_opt_next(codec->priv_data, option))) {
    switch (option->type) {
      case AV_OPT_TYPE_FLAGS:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: flags");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_INT: // based on x264 definitions
        ret = av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
        if (option->unit == nullptr) {
          // printf("For %s, return is %i value %i\n", option->name, ret, iValue);
          status = beam_set_int32(env, result, (char*) option->name, iValue);
          CHECK_STATUS;
        } else {
          if (iValue < 0) {
            status = beam_set_string_utf8(env, result, (char*) option->name, "unknown");
            CHECK_STATUS;
          } else {
            data = (uint8_t*) option->name;
            prev = option;
            option = av_opt_next(codec->priv_data, option);
            while (option->type == AV_OPT_TYPE_CONST) {
              if (option->default_val.i64 == iValue) {
                status = beam_set_string_utf8(env, result, (char*) data, (char*) option->name);
                CHECK_STATUS;
              }
              prev = option;
              option = av_opt_next(codec->priv_data, option);
            }
            option = prev;
          }
        }
        break;
      case AV_OPT_TYPE_INT64:
      case AV_OPT_TYPE_UINT64:
        ret = av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
        // printf("For %s, return is %i value %i\n", option->name, ret, iValue);
        status = beam_set_int64(env, result, (char*) option->name, iValue);
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_DOUBLE:
      case AV_OPT_TYPE_FLOAT:
        av_opt_get_double(codec->priv_data, option->name, 0, &dValue);
        status = beam_set_double(env, result, (char*) option->name, dValue);
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_STRING:
        av_opt_get(codec->priv_data, option->name, 0, &data);
        status = beam_set_string_utf8(env, result, (char*) option->name, (char*) data);
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_RATIONAL:
        av_opt_get_q(codec->priv_data, option->name, 0, &qValue);
        status = beam_set_rational(env, result, (char*) option->name, qValue);
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_BINARY:  ///< offset must point to a pointer immediately followed by an int for the length
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: binary");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_DICT:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: dict");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_CONST:
        // status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: const");
        // CHECK_STATUS;
        break;
      case AV_OPT_TYPE_IMAGE_SIZE: ///< offset must point to two consecutive integers
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: image_size");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_PIXEL_FMT:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: pixel_fmt");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_SAMPLE_FMT:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: sample_fmt");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_VIDEO_RATE: ///< offset must point to AVRational
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: AVRational");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_DURATION:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: duration");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_COLOR:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: color");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_CHANNEL_LAYOUT:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unmapped type: channel_layout");
        CHECK_STATUS;
        break;
      case AV_OPT_TYPE_BOOL:
        av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
        status = beam_set_bool(env, result, (char*) option->name, iValue);
        CHECK_STATUS;
        break;
      default:
        status = beam_set_string_utf8(env, result, (char*) option->name, "unknown type");
        CHECK_STATUS;
        break;
    }
  }

  return result;
}

napi_value setCodecCtxPrivData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value, names, element;
  napi_valuetype type;
  AVCodecContext* codec;
  bool isArray, flag;
  double dValue;
  uint32_t uThirtwo;
  char* sValue;
  const char* strProp;
  size_t sLen;
  const AVOption* option;
  int64_t iValue;
  int ret;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;

  if (codec->codec->priv_class == nullptr) {
    goto done;
  }

  if (argc == 0) {
    NAPI_THROW_ERROR("A value must be provided to set private_data.");
  }
  value = args[0];
  status = napi_typeof(env, value, &type);
  CHECK_STATUS;
  status = napi_is_array(env, value, &isArray);
  CHECK_STATUS;
  if ((isArray == false) && (type == napi_object)) {
    status = napi_get_property_names(env, value, &names);
    CHECK_STATUS;
    status = napi_get_array_length(env, names, &uThirtwo);
    CHECK_STATUS;
    for ( uint32_t x = 0 ; x < uThirtwo ; x++ ) {
      status = napi_get_element(env, names, x, &element);
      CHECK_STATUS;
      status = napi_get_value_string_utf8(env, element, nullptr, 0, &sLen);
      CHECK_STATUS;
      sValue = (char*) malloc(sizeof(char) * (sLen + 1));
      CHECK_STATUS;
      status = napi_get_value_string_utf8(env, element, sValue, sLen + 1, &sLen);
      CHECK_STATUS;
      option = av_opt_find(codec->priv_data, sValue, nullptr, 0, 0);
      if (option != nullptr) {
        status = napi_get_named_property(env, value, sValue, &element);
        CHECK_STATUS;
        status = napi_typeof(env, element, &type);
        CHECK_STATUS;
        switch (type) {
          case napi_boolean:
            status = napi_get_value_bool(env, element, &flag);
            CHECK_STATUS;
            ret = av_opt_set_int(codec->priv_data, sValue, flag, 0);
            if (ret < 0) printf("DEBUG: Unable to set %s with a boolean value.\n", sValue);
            break;
          case napi_number:
            if ((option->type == AV_OPT_TYPE_DOUBLE) ||
                (option->type == AV_OPT_TYPE_FLOAT)) {
              status = napi_get_value_double(env, element, &dValue);
              CHECK_STATUS;
              ret = av_opt_set_double(codec->priv_data, sValue, dValue, 0);
              if (ret < 0) printf("DEBUG: Unable to set %s with a double value %f.\n", sValue, dValue);
              break;
            }
            status = napi_get_value_int64(env, element, &iValue);
            CHECK_STATUS;
            ret = av_opt_set_int(codec->priv_data, sValue, iValue, 0);
            if (ret < 0) printf("DEBUG: Unable to set %s with an integer value %" PRId64 ".\n", sValue, iValue);
            break;
          case napi_string:
            status = napi_get_value_string_utf8(env, element, nullptr, 0, &sLen);
            CHECK_STATUS;
            strProp = (const char*) malloc(sizeof(char) * (sLen + 1));
            CHECK_STATUS;
            status = napi_get_value_string_utf8(env, element, (char*) strProp, sLen + 1, &sLen);
            CHECK_STATUS;
            ret = av_opt_set(codec->priv_data, sValue, strProp, 0);
            if (ret < 0) printf("DEBUG: Uable to set %s with a string value %s.\n", sValue, strProp);
            // TODO where does strProp get freed
            break;
          default:
            printf("DEBUG: Failed to set a private data value %s\n", sValue);
            break;
        }
      } else {
        printf("DEBUG: Option %s not found.\n", sValue);
      }
      free(sValue);
    }
  } else {
    NAPI_THROW_ERROR("An object with key/value pairs is required to set private data.");
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int64(env, codec->bit_rate, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the bit_rate property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the bit_rate property.");
  }

  status = napi_get_value_int64(env, args[0], &codec->bit_rate);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxBitRateTol(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->bit_rate_tolerance, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxBitRateTol(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the bit_rate_tolerance property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the bit_rate_tolerance property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->bit_rate_tolerance);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxGlobalQ(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->global_quality, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxGlobalQ(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the global_quality property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the global_quality property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->global_quality);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxCompLvl(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->compression_level, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxCompLvl(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the compression_level property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the compression_level property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->compression_level);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  /**
   * Allow decoders to produce frames with data planes that are not aligned
   * to CPU requirements (e.g. due to cropping).
   */
  status = beam_set_bool(env, result, "UNALIGNED", (codec->flags & AV_CODEC_FLAG_UNALIGNED) != 0);
  CHECK_STATUS;
  /**
   * Use fixed qscale.
   */
  status = beam_set_bool(env, result, "QSCALE", (codec->flags & AV_CODEC_FLAG_QSCALE) != 0);
  CHECK_STATUS;
  /**
   * 4 MV per MB allowed / advanced prediction for H.263.
   */
  status = beam_set_bool(env, result, "4MV", (codec->flags & AV_CODEC_FLAG_4MV) != 0);
  CHECK_STATUS;
  /**
   * Output even those frames that might be corrupted.
   */
  status = beam_set_bool(env, result, "OUTPUT_CORRUPT", (codec->flags & AV_CODEC_FLAG_OUTPUT_CORRUPT) != 0);
  CHECK_STATUS;
  /**
   * Use qpel MC.
   */
  status = beam_set_bool(env, result, "QPEL", (codec->flags & AV_CODEC_FLAG_QPEL) != 0);
  CHECK_STATUS;
  /**
   * Use internal 2pass ratecontrol in first pass mode.
   */
  status = beam_set_bool(env, result, "PASS1", (codec->flags & AV_CODEC_FLAG_PASS1) != 0);
  CHECK_STATUS;
  /**
   * Use internal 2pass ratecontrol in second pass mode.
   */
  status = beam_set_bool(env, result, "PASS2", (codec->flags & AV_CODEC_FLAG_PASS2) != 0);
  CHECK_STATUS;
  /**
   * loop filter.
   */
  status = beam_set_bool(env, result, "LOOP_FILTER", (codec->flags & AV_CODEC_FLAG_LOOP_FILTER) != 0);
  CHECK_STATUS;
  /**
   * Only decode/encode grayscale.
   */
  status = beam_set_bool(env, result, "GRAY", (codec->flags & AV_CODEC_FLAG_GRAY) != 0);
  CHECK_STATUS;
  /**
   * error[?] variables will be set during encoding.
   */
  status = beam_set_bool(env, result, "PSNR", (codec->flags & AV_CODEC_FLAG_PSNR) != 0);
  CHECK_STATUS;
  /**
   * Input bitstream might be truncated at a random location
   * instead of only at frame boundaries.
   */
  status = beam_set_bool(env, result, "TRUNCATED", (codec->flags & AV_CODEC_FLAG_TRUNCATED) != 0);
  CHECK_STATUS;
  /**
   * Use interlaced DCT.
   */
  status = beam_set_bool(env, result, "INTERLACED_DCT", (codec->flags & AV_CODEC_FLAG_INTERLACED_DCT) != 0);
  CHECK_STATUS;
  /**
   * Force low delay.
   */
  status = beam_set_bool(env, result, "LOW_DELAY", (codec->flags & AV_CODEC_FLAG_LOW_DELAY) != 0);
  CHECK_STATUS;
  /**
   * Place global headers in extradata instead of every keyframe.
   */
  status = beam_set_bool(env, result, "GLOBAL_HEADER", (codec->flags & AV_CODEC_FLAG_GLOBAL_HEADER) != 0);
  CHECK_STATUS;
  /**
   * Use only bitexact stuff (except (I)DCT).
   */
  status = beam_set_bool(env, result, "BITEXACT", (codec->flags & AV_CODEC_FLAG_BITEXACT) != 0);
  CHECK_STATUS;
  /* Fx : Flag for H.263+ extra options */
  /**
   * H.263 advanced intra coding / MPEG-4 AC prediction
   */
  status = beam_set_bool(env, result, "AC_PRED", (codec->flags & AV_CODEC_FLAG_AC_PRED) != 0);
  CHECK_STATUS;
  /**
   * interlaced motion estimation
   */
  status = beam_set_bool(env, result, "INTERLACED_ME", (codec->flags & AV_CODEC_FLAG_INTERLACED_ME) != 0);
  CHECK_STATUS;

  status = beam_set_bool(env, result, "CLOSED_GOP", (codec->flags & AV_CODEC_FLAG_CLOSED_GOP) != 0);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;
  napi_valuetype type;
  AVCodecContext* codec;
  bool isArray, present, flag;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the flags property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("An object of Boolean-valued flags is required to set the flags property.");
  }

  value = args[0];
  /**
   * Allow decoders to produce frames with data planes that are not aligned
   * to CPU requirements (e.g. due to cropping).
   */
  status = beam_get_bool(env, value, "UNALIGNED", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_UNALIGNED :
    codec->flags & ~AV_CODEC_FLAG_UNALIGNED; }
  /**
   * Use fixed qscale.
   */
  status = beam_get_bool(env, value, "QSCALE", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_QSCALE :
    codec->flags & ~AV_CODEC_FLAG_QSCALE; }
  /**
   * 4 MV per MB allowed / advanced prediction for H.263.
   */
  status = beam_get_bool(env, value, "4MV", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_4MV :
    codec->flags & ~AV_CODEC_FLAG_4MV; }
  /**
   * Output even those frames that might be corrupted.
   */
  status = beam_get_bool(env, value, "OUTPUT_CORRUPT", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_OUTPUT_CORRUPT :
    codec->flags & ~AV_CODEC_FLAG_OUTPUT_CORRUPT; }
  /**
   * Use qpel MC.
   */
  status = beam_get_bool(env, value, "QPEL", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_QPEL :
    codec->flags & ~AV_CODEC_FLAG_QPEL; }
  /**
   * Use internal 2pass ratecontrol in first pass mode.
   */
  status = beam_get_bool(env, value, "PASS1", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_PASS1 :
    codec->flags & ~AV_CODEC_FLAG_PASS1; }
  /**
   * Use internal 2pass ratecontrol in second pass mode.
   */
  status = beam_get_bool(env, value, "PASS2", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_PASS2 :
    codec->flags & ~AV_CODEC_FLAG_PASS2; }
  /**
   * loop filter.
   */
  status = beam_get_bool(env, value, "LOOP_FILTER", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_LOOP_FILTER :
    codec->flags & ~AV_CODEC_FLAG_LOOP_FILTER; }
  /**
   * Only decode/encode grayscale.
   */
  status = beam_get_bool(env, value, "GRAY", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_GRAY :
    codec->flags & ~AV_CODEC_FLAG_GRAY; }
  /**
   * error[?] variables will be set during encoding.
   */
  status = beam_get_bool(env, value, "PSNR", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_PSNR:
    codec->flags & ~AV_CODEC_FLAG_PSNR; }
  /**
   * Input bitstream might be truncated at a random location
   * instead of only at frame boundaries.
   */
  status = beam_get_bool(env, value, "TRUNCATED", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_TRUNCATED :
    codec->flags & ~AV_CODEC_FLAG_TRUNCATED; }
  /**
   * Use interlaced DCT.
   */
  status = beam_get_bool(env, value, "INTERLACED_DCT", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_INTERLACED_DCT :
    codec->flags & ~AV_CODEC_FLAG_INTERLACED_DCT; }
  /**
   * Force low delay.
   */
  status = beam_get_bool(env, value, "LOW_DELAY", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_LOW_DELAY :
    codec->flags & ~AV_CODEC_FLAG_LOW_DELAY; }
  /**
   * Place global headers in extradata instead of every keyframe.
   */
  status = beam_get_bool(env, value, "GLOBAL_HEADER", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_GLOBAL_HEADER :
    codec->flags & ~AV_CODEC_FLAG_GLOBAL_HEADER; }
  /**
   * Use only bitexact stuff (except (I)DCT).
   */
  status = beam_get_bool(env, value, "BITEXACT", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_BITEXACT :
    codec->flags & ~AV_CODEC_FLAG_BITEXACT; }
  /* Fx : Flag for H.263+ extra options */
  /**
   * H.263 advanced intra coding / MPEG-4 AC prediction
   */
  status = beam_get_bool(env, value, "AC_PRED", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_AC_PRED :
    codec->flags & ~AV_CODEC_FLAG_AC_PRED; }
  /**
   * interlaced motion estimation
   */
  status = beam_get_bool(env, value, "INTERLACED_ME", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_INTERLACED_ME :
    codec->flags & ~AV_CODEC_FLAG_INTERLACED_ME; }

  status = beam_get_bool(env, value, "CLOSED_GOP", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags = (flag) ?
    codec->flags | AV_CODEC_FLAG_CLOSED_GOP :
    codec->flags & ~AV_CODEC_FLAG_CLOSED_GOP; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxFlags2(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  /**
   * Allow non spec compliant speedup tricks.
   */
  status = beam_set_bool(env, result, "FAST", (codec->flags2 & AV_CODEC_FLAG2_FAST) != 0);
  CHECK_STATUS;
  /**
   * Skip bitstream encoding.
   */
  status = beam_set_bool(env, result, "NO_OUTPUT", (codec->flags2 & AV_CODEC_FLAG2_NO_OUTPUT) != 0);
  CHECK_STATUS;
  /**
   * Place global headers at every keyframe instead of in extradata.
   */
  status = beam_set_bool(env, result, "LOCAL_HEADER", (codec->flags2 & AV_CODEC_FLAG2_LOCAL_HEADER) != 0);
  CHECK_STATUS;

  /**
   * timecode is in drop frame format. DEPRECATED!!!!
   */
  status = beam_set_bool(env, result, "DROP_FRAME_TIMECODE", (codec->flags2 & AV_CODEC_FLAG2_DROP_FRAME_TIMECODE) != 0);
  CHECK_STATUS;

  /**
   * Input bitstream might be truncated at a packet boundaries
   * instead of only at frame boundaries.
   */
  status = beam_set_bool(env, result, "CHUNKS", (codec->flags2 & AV_CODEC_FLAG2_CHUNKS) != 0);
  CHECK_STATUS;
  /**
   * Discard cropping information from SPS.
   */
  status = beam_set_bool(env, result, "IGNORE_CROP", (codec->flags2 & AV_CODEC_FLAG2_IGNORE_CROP) != 0);
  CHECK_STATUS;

  /**
   * Show all frames before the first keyframe
   */
  status = beam_set_bool(env, result, "SHOW_ALL", (codec->flags2 & AV_CODEC_FLAG2_SHOW_ALL) != 0);
  CHECK_STATUS;
  /**
   * Export motion vectors through frame side data
   */
  status = beam_set_bool(env, result, "EXPORT_MVS", (codec->flags2 & AV_CODEC_FLAG2_EXPORT_MVS) != 0);
  CHECK_STATUS;
  /**
   * Do not skip samples and export skip information as frame side data
   */
  status = beam_set_bool(env, result, "SKIP_MANUAL", (codec->flags2 & AV_CODEC_FLAG2_SKIP_MANUAL) != 0);
  CHECK_STATUS;
  /**
   * Do not reset ASS ReadOrder field on flush (resulttitles decoding)
   */
  status = beam_set_bool(env, result, "RO_FLUSH_NOOP", (codec->flags2 & AV_CODEC_FLAG2_RO_FLUSH_NOOP) != 0);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxFlags2(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;
  napi_valuetype type;
  AVCodecContext* codec;
  bool isArray, present, flag;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the flags2 property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("An object of Boolean-valued flags is required to set the flags2 property.");
  }

  value = args[0];
  status = beam_get_bool(env, value, "FAST", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_FAST :
    codec->flags2 & ~AV_CODEC_FLAG2_FAST; }
  /**
   * Skip bitstream encoding.
   */
  status = beam_get_bool(env, value, "NO_OUTPUT", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_NO_OUTPUT :
    codec->flags2 & ~AV_CODEC_FLAG2_NO_OUTPUT; }
  /**
   * Place global headers at every keyframe instead of in extradata.
   */
  status = beam_get_bool(env, value, "LOCAL_HEADER", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_LOCAL_HEADER :
    codec->flags2 & ~AV_CODEC_FLAG2_LOCAL_HEADER; }
  /**
   * timecode is in drop frame format. DEPRECATED!!!!
   */
  status = beam_get_bool(env, value, "DROP_FRAME_TIMECODE", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_DROP_FRAME_TIMECODE :
    codec->flags2 & ~AV_CODEC_FLAG2_DROP_FRAME_TIMECODE; }
  /**
   * Input bitstream might be truncated at a packet boundaries
   * instead of only at frame boundaries.
   */
  status = beam_get_bool(env, value, "CHUNKS", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_CHUNKS :
    codec->flags2 & ~AV_CODEC_FLAG2_CHUNKS; }
  /**
   * Discard cropping information from SPS.
   */
  status = beam_get_bool(env, value, "IGNORE_CROP", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_IGNORE_CROP :
    codec->flags2 & ~AV_CODEC_FLAG2_IGNORE_CROP; }
  /**
   * Show all frames before the first keyframe
   */
  status = beam_get_bool(env, value, "SHOW_ALL", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_SHOW_ALL :
    codec->flags2 & ~AV_CODEC_FLAG2_SHOW_ALL; }
  /**
   * Export motion vectors through frame side data
   */
  status = beam_get_bool(env, value, "EXPORT_MVS", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_EXPORT_MVS :
    codec->flags2 & ~AV_CODEC_FLAG2_EXPORT_MVS; }
  /**
   * Do not skip samples and export skip information as frame side data
   */
  status = beam_get_bool(env, value, "SKIP_MANUAL", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_SKIP_MANUAL :
    codec->flags2 & ~AV_CODEC_FLAG2_SKIP_MANUAL; }
  /**
   * Do not reset ASS ReadOrder field on flush (subtitles decoding)
   */
  status = beam_get_bool(env, value, "RO_FLUSH_NOOP", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->flags2 = (flag) ?
    codec->flags2 | AV_CODEC_FLAG2_RO_FLUSH_NOOP :
    codec->flags2 & ~AV_CODEC_FLAG2_RO_FLUSH_NOOP; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxExtraData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;
  void* resultData;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (codec->extradata_size > 0) {
    status = napi_create_buffer_copy(env, codec->extradata_size, codec->extradata,
      &resultData, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecCtxExtraData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  bool isBuffer;
  AVCodecContext* codec;
  uint8_t* data;
  size_t dataLen;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the extradata property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type == napi_null) {
    if (codec->extradata_size > 0) { // Tidy up old buffers
      av_free(codec->extradata);
    }
    codec->extradata_size = 0;
    goto done;
  }
  status = napi_is_buffer(env, args[0], &isBuffer);
  CHECK_STATUS;
  if (!isBuffer) {
    NAPI_THROW_ERROR("A buffer is required to set the extradata property.");
  }

  status = napi_get_buffer_info(env, args[0], (void**) &data, &dataLen);
  CHECK_STATUS;
  if (codec->extradata_size > 0) { // Tidy up old buffers
    av_free(codec->extradata);
    codec->extradata_size = 0;
  }
  codec->extradata = (uint8_t*) av_malloc(dataLen + AV_INPUT_BUFFER_PADDING_SIZE);
  codec->extradata_size = dataLen;
  memcpy(codec->extradata, data, dataLen);

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxTimeBase(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->time_base.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->time_base.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxTimeBase(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the time_base property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("An array of two numbers is required to set the time_base property.");
  }
  for ( int x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("An array of two numbers is required to set the time_base property.");
    }
  }

  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &codec->time_base.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &codec->time_base.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxTicksPerFrame(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->ticks_per_frame, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxTicksPerFrame(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the ticks_per_frame property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the ticks_per_frame property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->ticks_per_frame);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxDelay(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->delay, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->width, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the width property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the width property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->width);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->height, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the height property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the height property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->height);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxCodedWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->coded_width, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxCodedWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the coded_width property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the coded_width property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->coded_width);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxCodedHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->coded_height, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxCodedHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the coded_height property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the coded_height property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->coded_height);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxGopSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->gop_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxGopSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the gop_size property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the gop_size property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->gop_size);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxPixFmt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;
  const char* pixFmtName;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;

  pixFmtName = av_get_pix_fmt_name(codec->pix_fmt);
  if (pixFmtName != nullptr) {
    status = napi_create_string_utf8(env,
      (char*) av_get_pix_fmt_name(codec->pix_fmt), NAPI_AUTO_LENGTH, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecCtxPixFmt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  AVPixelFormat pixFmt;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the width property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    codec->pix_fmt = AV_PIX_FMT_NONE;
    goto done;
  }
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the pix_fmt property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;

  pixFmt = av_get_pix_fmt((const char *) name);
  free(name);
  CHECK_STATUS;
  if (pixFmt != AV_PIX_FMT_NONE) {
    codec->pix_fmt = pixFmt;
  } else {
    NAPI_THROW_ERROR("Pixel format name is not known.");
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMaxBFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->max_b_frames, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMaxBFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the max_b_frames property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the max_b_frames property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->max_b_frames);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxBQuantFactor(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->b_quant_factor, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxBQuantFactor(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the b_quant_factor property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the b_quant_factor property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->b_quant_factor);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxBQuantOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->b_quant_offset, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxBQuantOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the b_quant_offset property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the b_quant_offset property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->b_quant_offset);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxHasBFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->has_b_frames, &result);
  CHECK_STATUS;

  return result;
}

napi_value getCodecCtxIQuantFactor(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->i_quant_factor, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxIQuantFactor(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the i_quant_factor property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the i_quant_factor property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->i_quant_factor);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxIQuantOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->i_quant_offset, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxIQuantOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the i_quant_offset property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the i_quant_offset property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->i_quant_factor);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxLumiMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->lumi_masking, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxLumiMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the lumi_masking property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the lumi_masking property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->lumi_masking);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxTempCplxMask(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->temporal_cplx_masking, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxTempCplxMask(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the temporal_cplx_masking property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the temporal_cplx_masking property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->temporal_cplx_masking);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxSpatCplxMask(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->spatial_cplx_masking, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxSpatCplxMask(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the spatial_cplx_masking property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the spatial_cplx_masking property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->spatial_cplx_masking);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxPMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->p_masking, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxPMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the p_masking property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the p_masking property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->p_masking);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxDarkMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_double(env, codec->p_masking, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxDarkMasking(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the dark_masking property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the dark_masking property.");
  }

  status = napi_get_value_double(env, args[0], (double*) &codec->dark_masking);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxSliceOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  if ((codec->slice_count > 0) && (codec->slice_offset != nullptr)) {
    status = napi_create_array(env, &result);
    CHECK_STATUS;
    for ( int x = 0 ; x < codec->slice_count ; x++ ) {
      status = napi_create_int32(env, codec->slice_offset[x], &element);
      CHECK_STATUS;
      status = napi_set_element(env, result, x, element);
      CHECK_STATUS;
    }
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecCtxSliceOffset(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVCodecContext* codec;
  int sliceCount;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the slice_offset property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    codec->slice_count = 0;
    codec->slice_offset = nullptr;
    goto done;
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("An array of numbers is required to set the slice_offset property.");
  }
  status = napi_get_array_length(env, args[0], (uint32_t*) &sliceCount);
  CHECK_STATUS;
  for ( int x = 0 ; x < sliceCount ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("Only values of number type can be used to set the slice_offset property.");
    }
  }

  codec->slice_count = sliceCount;
  codec->slice_offset = (int*) av_malloc(sizeof(int) * sliceCount);
  for ( int x = 0 ; x < sliceCount ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_get_value_int32(env, element, &codec->slice_offset[x]);
    CHECK_STATUS;
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxSampleAspRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->sample_aspect_ratio.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->sample_aspect_ratio.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxSampleAspRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the sample_aspect_ratio property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("An array of two numbers is required to set the sample_aspect_ratio property.");
  }
  for ( int x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("All array elements must be numbers to set the sample_aspect_ratio property.");
    }
  }

  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &codec->sample_aspect_ratio.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &codec->sample_aspect_ratio.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMeCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_cmp->forward, codec->me_cmp), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMeCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the me_cmp (motion estimation comparison function) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the me_cmp (motion estimation comparison function) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_cmp->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->me_cmp = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for me_cmp (motion estimation comparison function).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMeSubCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_cmp->forward, codec->me_sub_cmp), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMeSubCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the me_sub_cmp (subpixel motion estimation comparison function) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the me_sub_cmp (subpixel motion estimation comparison function) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_cmp->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->me_sub_cmp = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for me_subcmp (subpixelmotion estimation comparison function).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMbCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_cmp->forward, codec->mb_cmp), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMbCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the mb_cmp (macroblock comparison function) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the mb_cmp (macroblock comparison function) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_cmp->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->mb_cmp = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for mb_cmp (macroblock comparison function).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxIldctCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_cmp->forward, codec->ildct_cmp), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxIldctCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the ildct_cmp (interlaced DCT comparison function) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the ildct_cmp (interlaced DCT comparison function) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_cmp->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->ildct_cmp = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for ildct_cmp (interlaced DCT comparison function).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxDiaSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->dia_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxDiaSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the dia_size property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the dia_size property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->dia_size);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxLastPredCount(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->last_predictor_count, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxLastPredCount(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the last_predictor_count property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the last_predictor_count property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->last_predictor_count);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMePreCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_cmp->forward, codec->me_pre_cmp), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMePreCmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the me_pre_cmp (motion estimation prepass comparison function) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the me_pre_cmp (motion estimation prepass comparison function) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_cmp->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->me_pre_cmp = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for me_pre_cmp (motion estimation prepass comparison function).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxPreDiaSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->pre_dia_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxPreDiaSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the pre_dia_size property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the pre_dia_size property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->pre_dia_size);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMeSubpelQual(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->pre_dia_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMeSubpelQual(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the me_subpel_quality property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the me_subpel_quality property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->me_subpel_quality);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMeRange(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_int32(env, codec->me_range, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMeRange(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the me_range property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("A number is required to set the me_range property.");
  }

  status = napi_get_value_int32(env, args[0], &codec->me_range);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxSliceFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  // draw_horiz_band() is called in coded order instead of display
  status = beam_set_bool(env, result, "CODED_ORDER", (codec->slice_flags & SLICE_FLAG_CODED_ORDER) != 0);
  CHECK_STATUS;
  // allow draw_horiz_band() with field slices (MPEG-2 field pics)
  status = beam_set_bool(env, result, "ALLOW_FIELD", (codec->slice_flags & SLICE_FLAG_ALLOW_FIELD) != 0);
  CHECK_STATUS;
  // allow draw_horiz_band() with 1 component at a time (SVQ1)
  status = beam_set_bool(env, result, "ALLOW_PLANE", (codec->slice_flags & SLICE_FLAG_ALLOW_PLANE) != 0);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxSliceFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;
  napi_valuetype type;
  AVCodecContext* codec;
  bool isArray, present, flag;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the slice_flags property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("An object of Boolean-valued flags is required to set the slice_flags property.");
  }

  value = args[0];
  // draw_horiz_band() is called in coded order instead of display
  status = beam_get_bool(env, value, "CODED_ORDER", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->slice_flags = (flag) ?
    codec->slice_flags | SLICE_FLAG_CODED_ORDER :
    codec->slice_flags & ~SLICE_FLAG_CODED_ORDER; }
  // allow draw_horiz_band() with field slices (MPEG-2 field pics)
  status = beam_get_bool(env, value, "ALLOW_FIELD", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->slice_flags = (flag) ?
    codec->slice_flags | SLICE_FLAG_ALLOW_FIELD :
    codec->slice_flags & ~SLICE_FLAG_ALLOW_FIELD; }
  // allow draw_horiz_band() with 1 component at a time (SVQ1)
  status = beam_get_bool(env, value, "ALLOW_PLANE", &present, &flag);
  CHECK_STATUS;
  if (present) { codec->slice_flags = (flag) ?
    codec->slice_flags | SLICE_FLAG_ALLOW_PLANE :
    codec->slice_flags & ~SLICE_FLAG_ALLOW_PLANE; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxMbDecision(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_ff_mb_decision->forward, codec->mb_decision), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecCtxMbDecision(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVCodecContext* codec;
  char* name;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the mb_decision (macroblock decision mode) property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("A string is required to set the mb_decision (macroblock decision mode) property.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], name, strLen + 1, &strLen);
  CHECK_STATUS;
  enumValue = beam_lookup_enum(beam_ff_mb_decision->inverse, name);
  free(name);
  if (enumValue != BEAM_ENUM_UNKNOWN) {
    codec->mb_decision = enumValue;
  } else {
    NAPI_THROW_ERROR("Unknown function for mb_decision (macroblock decision mode).");
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxIntraMatrix(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (codec->intra_matrix != nullptr) {
    status = napi_create_array(env, &result);
    CHECK_STATUS;
    for ( int x = 0 ; x < 64 ; x++ ) {
      status = napi_create_uint32(env, codec->intra_matrix[x], &element);
      CHECK_STATUS;
      status = napi_set_element(env, result, x, element);
      CHECK_STATUS;
    }
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecCtxIntraMatrix(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVCodecContext* codec;
  uint32_t uThirtwo;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the intra_matrix property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    codec->intra_matrix = nullptr;
    goto done;
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("An array of numbers is required to set the intra_matrix property.");
  }
  codec->intra_matrix = (uint16_t*) av_mallocz(sizeof(uint16_t) * 64);
  for ( int x = 0 ; x < 64 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type == napi_number) {
      status = napi_get_value_uint32(env, element, &uThirtwo);
      CHECK_STATUS;
      codec->intra_matrix[x] = (uint16_t) uThirtwo;
    } else {
      codec->intra_matrix[x] = 0;
    }
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecCtxInterMatrix(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVCodecContext* codec;

  size_t argc = 0;
  status = napi_get_cb_info(env, info, &argc, nullptr, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (codec->inter_matrix != nullptr) {
    status = napi_create_array(env, &result);
    CHECK_STATUS;
    for ( int x = 0 ; x < 64 ; x++ ) {
      status = napi_create_uint32(env, codec->inter_matrix[x], &element);
      CHECK_STATUS;
      status = napi_set_element(env, result, x, element);
      CHECK_STATUS;
    }
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecCtxInterMatrix(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVCodecContext* codec;
  uint32_t uThirtwo;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &codec);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the inter_matrix property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    codec->intra_matrix = nullptr;
    goto done;
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("An array of numbers is required to set the inter_matrix property.");
  }
  codec->inter_matrix = (uint16_t*) av_mallocz(sizeof(uint16_t) * 64);
  for ( int x = 0 ; x < 64 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type == napi_number) {
      status = napi_get_value_uint32(env, element, &uThirtwo);
      CHECK_STATUS;
      codec->inter_matrix[x] = (uint16_t) uThirtwo;
    } else {
      codec->inter_matrix[x] = 0;
    }
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value failDecoding(napi_env env, napi_callback_info info) {
  NAPI_THROW_ERROR("Cannot set property when decoding.");
}

napi_value failEncoding(napi_env env, napi_callback_info info) {
  NAPI_THROW_ERROR("Cannot set property when encoding.");
}

napi_value failBoth(napi_env env, napi_callback_info info) {
  NAPI_THROW_ERROR("User cannot set this property.");
}

napi_status fromAVCodecContext(napi_env env, AVCodecContext* codec,
    napi_value* result, bool encoding) {
  napi_status status;
  napi_value jsCodec, extCodec, typeName, undef;

  status = napi_create_object(env, &jsCodec);
  PASS_STATUS;
  status = napi_create_string_utf8(env, encoding ? "encoder" : "decoder",
    NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_get_undefined(env, &undef);
  PASS_STATUS;
  status = napi_create_external(env, codec, codecContextFinalizer, nullptr, &extCodec);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "codec_id", nullptr, nullptr, getCodecCtxCodecID, nop, nullptr,
      napi_enumerable, codec},
    { "name", nullptr, nullptr, getCodecCtxName, nop, nullptr,
      napi_enumerable, codec},
    { "long_name", nullptr, nullptr, getCodecCtxLongName, nullptr, nullptr,
      napi_enumerable, codec},
    { "codec_tag", nullptr, nullptr, getCodecCtxCodecTag, nullptr, nullptr,
      napi_enumerable, codec},
    { "priv_data", nullptr, nullptr, getCodecCtxPrivData, setCodecCtxPrivData, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    // TODO internal
    // TODO opaque
    { "bit_rate", nullptr, nullptr, getCodecCtxBitRate, setCodecCtxBitRate, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "bit_rate_tolerance", nullptr, nullptr, getCodecCtxBitRateTol,
      encoding ? setCodecCtxBitRateTol : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "global_quality", nullptr, nullptr, getCodecCtxGlobalQ,
      encoding ? setCodecCtxGlobalQ : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    // 10
    { "compression_level", nullptr, nullptr, getCodecCtxCompLvl,
      encoding ? setCodecCtxCompLvl : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "flags", nullptr, nullptr, getCodecCtxFlags, setCodecCtxFlags, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "flags2", nullptr, nullptr, getCodecCtxFlags2, setCodecCtxFlags2, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "extradata", nullptr, nullptr, getCodecCtxExtraData,
      encoding ? failEncoding : setCodecCtxExtraData, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "time_base", nullptr, nullptr, getCodecCtxTimeBase,
       encoding ? setCodecCtxTimeBase : failDecoding, nullptr,
       (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "ticks_per_frame", nullptr, nullptr, getCodecCtxTicksPerFrame, setCodecCtxTicksPerFrame, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "delay", nullptr, nullptr, getCodecCtxDelay, failBoth, nullptr,
       napi_enumerable, codec},
    { "width", nullptr, nullptr, getCodecCtxWidth, setCodecCtxWidth, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "height", nullptr, nullptr, getCodecCtxHeight, setCodecCtxHeight, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "coded_width", nullptr, nullptr,
      encoding ? nullptr : getCodecCtxCodedWidth,
      encoding ? failEncoding : setCodecCtxCodedWidth, nullptr,
      encoding ? napi_default : (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    // 20
    { "coded_height", nullptr, nullptr,
      encoding ? nullptr : getCodecCtxCodedHeight,
      encoding ? failEncoding : setCodecCtxCodedHeight, nullptr,
      encoding ? napi_default : (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "gop_size" , nullptr, nullptr,
       encoding ? getCodecCtxGopSize : nullptr,
       encoding ? setCodecCtxGopSize : failDecoding, nullptr,
       encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "pix_fmt", nullptr, nullptr, getCodecCtxPixFmt, setCodecCtxPixFmt, nullptr,
       (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "max_b_frames", nullptr, nullptr,
      encoding ? getCodecCtxMaxBFrames : nullptr,
      encoding ? setCodecCtxMaxBFrames : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "b_quant_factor", nullptr, nullptr,
      encoding ? getCodecCtxBQuantFactor : nullptr,
      encoding ? setCodecCtxBQuantFactor : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "b_quant_offset", nullptr, nullptr,
      encoding ? getCodecCtxBQuantOffset : nullptr,
      encoding ? setCodecCtxBQuantOffset : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "has_b_frames", nullptr, nullptr, getCodecCtxHasBFrames, failBoth, nullptr,
       napi_enumerable, codec},
    { "i_quant_factor", nullptr, nullptr,
      encoding ? getCodecCtxIQuantFactor : nullptr,
      encoding ? setCodecCtxIQuantFactor : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "i_quant_offset", nullptr, nullptr,
      encoding ? getCodecCtxIQuantOffset : nullptr,
      encoding ? setCodecCtxIQuantOffset : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "lumi_masking", nullptr, nullptr,
      encoding ? getCodecCtxLumiMasking : nullptr,
      encoding ? setCodecCtxLumiMasking : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    // 30
    { "temporal_cplx_masking", nullptr, nullptr,
      encoding ? getCodecCtxTempCplxMask : nullptr,
      encoding ? setCodecCtxTempCplxMask : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "spatial_cplx_masking", nullptr, nullptr,
      encoding ? getCodecCtxSpatCplxMask : nullptr,
      encoding ? setCodecCtxSpatCplxMask : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "p_masking", nullptr, nullptr,
      encoding ? getCodecCtxPMasking : nullptr,
      encoding ? setCodecCtxPMasking : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "dark_masking", nullptr, nullptr,
      encoding ? getCodecCtxDarkMasking : nullptr,
      encoding ? setCodecCtxDarkMasking : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "slice_offset", nullptr, nullptr, getCodecCtxSliceOffset,
      encoding ? failEncoding : setCodecCtxSliceOffset, nullptr,
      encoding ? napi_enumerable : (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "sample_aspect_ratio", nullptr, nullptr, getCodecCtxSampleAspRt,
      encoding ? setCodecCtxSampleAspRt : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_enumerable, codec},
    { "me_cmp", nullptr, nullptr,
      encoding ? getCodecCtxMeCmp: nullptr,
      encoding ? setCodecCtxMeCmp : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "me_sub_cmp", nullptr, nullptr,
      encoding ? getCodecCtxMeSubCmp: nullptr,
      encoding ? setCodecCtxMeSubCmp : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "mb_cmp", nullptr, nullptr,
      encoding ? getCodecCtxMbCmp: nullptr,
      encoding ? setCodecCtxMbCmp : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "ildct_cmp", nullptr, nullptr,
      encoding ? getCodecCtxIldctCmp: nullptr,
      encoding ? setCodecCtxIldctCmp : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    // 40
    { "dia_size", nullptr, nullptr,
      encoding ? getCodecCtxDiaSize: nullptr,
      encoding ? setCodecCtxDiaSize : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "last_predictor_count", nullptr, nullptr,
      encoding ? getCodecCtxLastPredCount: nullptr,
      encoding ? setCodecCtxLastPredCount : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "mb_pre_cmp", nullptr, nullptr,
      encoding ? getCodecCtxMePreCmp : nullptr,
      encoding ? setCodecCtxMePreCmp : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "pre_dia_size", nullptr, nullptr,
      encoding ? getCodecCtxPreDiaSize: nullptr,
      encoding ? setCodecCtxPreDiaSize : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "me_sbupel_quality", nullptr, nullptr,
      encoding ? getCodecCtxMeSubpelQual : nullptr,
      encoding ? setCodecCtxMeSubpelQual : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "me_range", nullptr, nullptr,
      encoding ? getCodecCtxMeRange : nullptr,
      encoding ? setCodecCtxMeRange : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "slice_flags", nullptr, nullptr,
      encoding ? nullptr : getCodecCtxSliceFlags,
      encoding ? failEncoding : setCodecCtxSliceFlags, nullptr,
      encoding ? napi_default : (napi_property_attributes) (napi_writable | napi_enumerable), codec},
    { "mb_decision", nullptr, nullptr,
      encoding ? getCodecCtxMbDecision : nullptr,
      encoding ? setCodecCtxMbDecision : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_default, codec},
    { "intra_matrix", nullptr, nullptr, getCodecCtxIntraMatrix,
      encoding ? setCodecCtxIntraMatrix : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_enumerable, codec},
    { "inter_matrix", nullptr, nullptr, getCodecCtxInterMatrix,
      encoding ? setCodecCtxInterMatrix : failDecoding, nullptr,
      encoding ? (napi_property_attributes) (napi_writable | napi_enumerable) : napi_enumerable, codec},
    // 50
    { "_CodecContext", nullptr, nullptr, nullptr, nullptr, extCodec, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsCodec, 50, desc);
  PASS_STATUS;

  *result = jsCodec;
  return napi_ok;
}

void codecContextFinalizer(napi_env env, void* data, void* hint) {
  AVCodecContext* codecCtx = (AVCodecContext*) data;
  if (codecCtx->extradata != nullptr) {
    av_free(codecCtx->extradata);
    codecCtx->extradata_size = 0;
  }
  avcodec_close(codecCtx);
  avcodec_free_context(&codecCtx);
}