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

#include "codec_par.h"

napi_value getCodecParCodecType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    av_get_media_type_string(c->codecPars->codec_type), NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParCodecType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  int mtype;
  char * typeName;
  size_t typeNameLen;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters codec_type must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameter codec_type must be set using a string.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &typeNameLen);
  CHECK_STATUS;
  typeName = (char*) malloc(sizeof(char) * (typeNameLen + 1));
  status = napi_get_value_string_utf8(env, args[0], typeName, typeNameLen + 1, &typeNameLen);
  CHECK_STATUS;

  mtype = beam_lookup_enum(beam_avmedia_type->inverse, typeName);
  c->codecPars->codec_type = (mtype == BEAM_ENUM_UNKNOWN) ?
    AVMEDIA_TYPE_UNKNOWN : (AVMediaType) mtype;

//SET_BODY codec_type AVMediaType CodecType
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParCodecID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->codec_id, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParCodecID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters codec_id must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameters codec_id must be set by a number.");
  }

  status = napi_get_value_int32(env, args[0], (int32_t*) &c->codecPars->codec_id);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, (char*) avcodec_get_type(c->codecPars->codec_id),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* codecName;
  size_t nameLen;
  const AVCodecDescriptor* codecDesc;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters name must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters name must be provided with a value.");
  }

  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &nameLen);
  CHECK_STATUS;
  codecName = (char*) malloc(sizeof(char) * (nameLen + 1));
  status = napi_get_value_string_utf8(env, args[0], codecName, nameLen + 1, &nameLen);
  CHECK_STATUS;
  codecDesc = avcodec_descriptor_get_by_name((const char *) codecName);
  CHECK_STATUS;
  if (codecDesc == nullptr) {
    NAPI_THROW_ERROR("Codec parameter codec_name does not match a known codec.");
  }
  c->codecPars->codec_id = codecDesc->id;

  free(codecName);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParCodecTag(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  char fourcc[AV_FOURCC_MAX_STRING_SIZE];

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  av_fourcc_make_string(fourcc, c->codecPars->codec_tag);
  status = napi_create_string_utf8(env, fourcc, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

// Not setable - derived from codec id
/* napi_value setCodecParCodecTag(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters codec_tag must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;

//SET_BODY codec_tag uint32_t CodecTag
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
} */

napi_value getCodecParFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* fmtName = nullptr;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  switch (c->codecPars->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      fmtName = av_get_pix_fmt_name((AVPixelFormat) c->codecPars->format);
      break;
    case AVMEDIA_TYPE_AUDIO:
      fmtName = av_get_sample_fmt_name((AVSampleFormat) c->codecPars->format);
      break;
    default: // Might not have media type set
      fmtName = av_get_pix_fmt_name((AVPixelFormat) c->codecPars->format);
      if (fmtName == nullptr) {
        fmtName = av_get_sample_fmt_name((AVSampleFormat) c->codecPars->format);
      }
      break;
  }
  if (fmtName == nullptr) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    status = napi_create_string_utf8(env, (char*) fmtName, NAPI_AUTO_LENGTH, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setCodecParFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* formatName;
  size_t nameLen;
  int format;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters format must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters format must be set with a string value");
  }

  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &nameLen);
  CHECK_STATUS;
  formatName = (char*) malloc(sizeof(char) * (nameLen + 1));
  status = napi_get_value_string_utf8(env, args[0], formatName, nameLen + 1, &nameLen);
  CHECK_STATUS;

  switch (c->codecPars->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      format = (int) av_get_pix_fmt((const char *) formatName);
      break;
    case AVMEDIA_TYPE_AUDIO:
      format = (int) av_get_sample_fmt((const char *) formatName);
      break;
    default: // codec_type may not have been set yet ... guess mode
      format = (int) av_get_pix_fmt((const char *) formatName);
      if (format == AV_PIX_FMT_NONE) {
        format = (int) av_get_sample_fmt((const char *) formatName);
      }
      break;
  }
  c->codecPars->format = format;

  free(formatName);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int64(env, c->codecPars->bit_rate, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters bit_rate must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameters bit_rate must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &c->codecPars->bit_rate);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParSmpAspectRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, c->codecPars->sample_aspect_ratio.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, c->codecPars->sample_aspect_ratio.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParSmpAspectRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, num, den;
  napi_valuetype type;
  codecParData* c;
  bool isArray;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters sample_aspect_ratio must be provided with a value.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("Codec parameters sample_aspect_ratio must be set with an array of numbers.");
  }

  status = napi_get_element(env, args[0], 0, &num);
  CHECK_STATUS;
  status = napi_typeof(env, num, &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameters sample_aspect_ratio numerator must be a number.");
  }
  CHECK_STATUS;

  status = napi_get_element(env, args[0], 0, &den);
  CHECK_STATUS;
  status = napi_typeof(env, den, &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameters sample_aspect_ratio denominator must be a number.");
  }
  CHECK_STATUS;

  status = napi_get_value_int32(env, num, &c->codecPars->sample_aspect_ratio.num);
  CHECK_STATUS;
  status = napi_get_value_int32(env, den, &c->codecPars->sample_aspect_ratio.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParFieldOrder(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (char*) beam_lookup_name(beam_field_order->forward, c->codecPars->field_order),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParFieldOrder(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters field_order must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters field_order must be set with a string.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = beam_lookup_enum(beam_field_order->inverse, enumString);
  if (enumValue == BEAM_ENUM_UNKNOWN) {
    NAPI_THROW_ERROR("Codec parameters field_order value unrecognised. Did you mean e.g. 'progressive'?")
  }
  c->codecPars->field_order = (AVFieldOrder) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParColorRange(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* enumName;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  enumName = av_color_range_name(c->codecPars->color_range);
  status = napi_create_string_utf8(env,
    (enumName != nullptr) ? (char*) enumName : "unknown", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParColorRange(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters color_range must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters color_range must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = av_color_range_from_name((const char *) enumString);
  if (enumValue < 0) {
    NAPI_THROW_ERROR("Codec parameter color_range is not recognised. One of 'tv' or 'pc'?");
  }
  c->codecPars->color_range = (AVColorRange) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParColorPrims(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* enumName;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  enumName = av_color_primaries_name(c->codecPars->color_primaries);
  status = napi_create_string_utf8(env,
    (enumName != nullptr) ? (char*) enumName : "unknown",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParColorPrims(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters color_primaries must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters color_primaries must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = av_color_primaries_from_name((const char *) enumString);
  if (enumValue < 0) {
    NAPI_THROW_ERROR("Codec parameter color_primaries is not recognised. Did you mean e.g. 'bt709'?");
  }
  c->codecPars->color_primaries = (AVColorPrimaries) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParColorTrc(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* enumName;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  enumName = av_color_transfer_name(c->codecPars->color_trc);
  status = napi_create_string_utf8(env,
    (enumName != nullptr) ? (char*) enumName : "unknown",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParColorTrc(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters color_trc must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters color_trc must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = av_color_transfer_from_name((const char *) enumString);
  if (enumValue < 0) {
    NAPI_THROW_ERROR("Codec parameter color_trc is not recognised. Did you mean e.g. 'bt709'?");
  }
  c->codecPars->color_trc = (AVColorTransferCharacteristic) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParColorSpace(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* enumName;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  enumName = av_color_space_name(c->codecPars->color_space);
  status = napi_create_string_utf8(env,
    (enumName != nullptr) ? (char*) enumName : "unknown",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParColorSpace(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters color_space must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters color_space must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = av_color_space_from_name((const char *) enumString);
  if (enumValue < 0) {
    NAPI_THROW_ERROR("Codec parameter color_space is not recognised. Did you mean e.g. 'bt709'?");
  }
  c->codecPars->color_space = (AVColorSpace) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParChromaLoc(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;
  const char* enumName;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  enumName = av_chroma_location_name(c->codecPars->chroma_location);
  status = napi_create_string_utf8(env,
    (enumName != nullptr) ? (char*) enumName : "unspecified",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setCodecParChromaLoc(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;
  char* enumString;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameters chroma_location must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Codec parameters chroma_location must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumString = (char*) malloc(sizeof(char) * (strLen + 1));
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, args[0], enumString, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = av_chroma_location_from_name((const char *) enumString);
  if (enumValue < 0) {
    NAPI_THROW_ERROR("Codec parameter chroma_location is not recognised. Did you mean e.g. 'left'?");
  }
  c->codecPars->chroma_location = (AVChromaLocation) enumValue;

  free(enumString);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParChanLayout(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

//GET_BODY channel_layout uint64_t ChanLayout
  return nullptr;
}

napi_value setCodecParChanLayout(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {  NAPI_THROW_ERROR("Codec parameters channel_layout must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;

//SET_BODY channel_layout uint64_t ChanLayout
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}


napi_value getCodecParBitsPerCodedSmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->bits_per_coded_sample, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParBitsPerCodedSmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter bits_per_coded_sample must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter bits_per_coded_sample must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->bits_per_coded_sample);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParBitsPerRawSmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->bits_per_raw_sample, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParBitsPerRawSmp(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter bits_per_raw_sample must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter bits_per_raw_sample must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->bits_per_raw_sample);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParLevel(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->level, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParLevel(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter level must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter level must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->level);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->width, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter width must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter width must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->width);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->height, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter height must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter height must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->height);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParVideoDelay(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->video_delay, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParVideoDelay(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter video_delay must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter video_delay must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->video_delay);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}


napi_value getCodecParChannels(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->channels, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParChannels(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter channels must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter channels must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->channels);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParSampleRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->sample_rate, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParSampleRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter sample_rate must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter sample_rate must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->sample_rate);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParBlockAlign(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->block_align, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParBlockAlign(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter block_align must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter block_align must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->block_align);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}


napi_value getCodecParFrameSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->frame_size, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParFrameSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter frame_size must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter frame_size must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->frame_size);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParInitialPad(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->initial_padding, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParInitialPad(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter initial_padding must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter initial_padding must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->initial_padding);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParTrailingPad(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->trailing_padding, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParTrailingPad(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter trailing_padding must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter trailing_padding must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->trailing_padding);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getCodecParSeekPreroll(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  codecParData* c;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &c);
  CHECK_STATUS;

  status = napi_create_int32(env, c->codecPars->seek_preroll, &result);
  CHECK_STATUS;
  return result;
}

napi_value setCodecParSeekPreroll(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  codecParData* c;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &c);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Codec parameter seek_preroll must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Codec parameter seek_preroll must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &c->codecPars->seek_preroll);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value makeCodecParameters(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, global, jsObject, assign;
  napi_valuetype type;
  bool isArray;
  codecParData* c = new codecParData;
  c->codecPars = avcodec_parameters_alloc();

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc > 1) {
    NAPI_THROW_ERROR("Parameters may be created with zero or one options object argument.");
  }
  if (argc == 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    status = napi_is_array(env, args[0], &isArray);
    CHECK_STATUS;
    if (isArray || (type != napi_object)) {
      NAPI_THROW_ERROR("Cannot create codec parameters unless argument is an object.");
    }
  }

  status = fromAVCodecParameters(env, c, &result);
  CHECK_STATUS;

  if (argc == 1) {
    status = napi_get_global(env, &global);
    CHECK_STATUS;
    status = napi_get_named_property(env, global, "Object", &jsObject);
    CHECK_STATUS;
    status = napi_get_named_property(env, jsObject, "assign", &assign);
    CHECK_STATUS;
    const napi_value fargs[] = { result, args[0] };
    status = napi_call_function(env, result, assign, 2, fargs, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_status fromAVCodecParameters(napi_env env, codecParData* c, napi_value* result) {
  napi_status status;
  napi_value jsCodecPar, extCodecPar, typeName;

  status = napi_create_object(env, &jsCodecPar);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "CodecParameters", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_create_external(env, c, codecParDataFinalizer, nullptr, &extCodecPar);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "_codecPar", nullptr, nullptr, nullptr, nullptr, extCodecPar, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsCodecPar, 2, desc);
  PASS_STATUS;

  *result = jsCodecPar;
  return napi_ok;
};

void codecParDataFinalizer(napi_env env, void* data, void* hint) {
  codecParData* c = (codecParData*) data;
  delete c;
}
