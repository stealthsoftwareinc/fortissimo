/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {

template<typename Identity_T>
template<typename Value_T>
bool IncomingMessage<Identity_T>::read(Value_T & result) {
  return msg_read(*this, result);
}

template<typename Identity_T>
template<typename Value_T>
bool OutgoingMessage<Identity_T>::write(Value_T const & result) {
  return msg_write(*this, result);
}

static_assert(
    std::numeric_limits<uint8_t>::digits == CHAR_BIT && CHAR_BIT == 8,
    "Bits per byte constant incorrect.");

template<typename Identity_T, typename Value_T>
bool msg_read_int(IncomingMessage<Identity_T> & msg, Value_T & result) {
  static_assert(::std::is_integral<Value_T>::value, "Unsupported type");

  const size_t integerSize = sizeof(Value_T);
  const size_t topByteIndex = integerSize - 1;
  ::std::array<uint8_t, sizeof(Value_T)> bytes;
  bool ret = msg.remove(bytes.data(), integerSize) == integerSize;

  result = 0;

  for (size_t it = 0; it < integerSize; it++) {
    const size_t shiftPosition =
        (topByteIndex - it) * std::numeric_limits<uint8_t>::digits;
    result = result | (Value_T)((Value_T)bytes[it] << shiftPosition);
  }

  return ret;
}

template<typename Identity_T, typename Value_T>
bool msg_write_int(
    OutgoingMessage<Identity_T> & msg, const Value_T value) {
  static_assert(::std::is_integral<Value_T>::value, "Unsupported type");
  const size_t integerSize = sizeof(Value_T);
  const size_t topByteIndex = integerSize - 1;
  ::std::array<uint8_t, sizeof(Value_T)> result;

  for (size_t it = 0; it < integerSize; it++) {
    const size_t shiftPosition =
        (topByteIndex - it) * std::numeric_limits<uint8_t>::digits;
    result[it] = (uint8_t)(value >> shiftPosition);
  }

  return msg.add(result.data(), integerSize) == integerSize;
}

template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, uint8_t & val) {
  return msg_read_int<Identity_T, uint8_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, uint16_t & val) {
  return msg_read_int<Identity_T, uint16_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, uint32_t & val) {
  return msg_read_int<Identity_T, uint32_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, uint64_t & val) {
  return msg_read_int<Identity_T, uint64_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, int8_t & val) {
  return msg_read_int<Identity_T, int8_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, int16_t & val) {
  return msg_read_int<Identity_T, int16_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, int32_t & val) {
  return msg_read_int<Identity_T, int32_t>(msg, val);
}
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, int64_t & val) {
  return msg_read_int<Identity_T, int64_t>(msg, val);
}

template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, uint8_t const val) {
  return msg_write_int<Identity_T, uint8_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, uint16_t const val) {
  return msg_write_int<Identity_T, uint16_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, uint32_t const val) {
  return msg_write_int<Identity_T, uint32_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, uint64_t const val) {
  return msg_write_int<Identity_T, uint64_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, int8_t const val) {
  return msg_write_int<Identity_T, int8_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, int16_t const val) {
  return msg_write_int<Identity_T, int16_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, int32_t const val) {
  return msg_write_int<Identity_T, int32_t>(msg, val);
}
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> & msg, int64_t const val) {
  return msg_write_int<Identity_T, int64_t>(msg, val);
}

template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, ::std::string & val) {
  uint32_t len32;
  if (!msg.template read<uint32_t>(len32)) {
    return false;
  }
  val.resize((size_t)len32);

  return msg.remove(&val[0], (size_t)len32) == (size_t)len32;
}
template<typename Identity_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg, ::std::string const & val) {
  if (!msg.template write<uint32_t>((uint32_t)val.size())) {
    return false;
  }
  return msg.add(val.data(), val.size()) == val.size();
}

template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> & msg, sst::bignum & bn) {
  // Hopefully it is a safe assumption that we'll never need bignums
  // larger than 2^(8*65000)
  uint16_t len_16 = 0;
  bool success = msg.template read<uint16_t>(len_16);
  size_t len = (size_t)len_16;

  uint8_t buf[len];
  success = success && msg.remove(buf, len) == len;

  success = success && nullptr != BN_bin2bn(buf, (int)len, bn.peek());

  return success;
}

template<typename Identity_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg, sst::bignum const & bn) {
  size_t bn_len = (size_t)BN_num_bytes(bn.peek());
  size_t n_bytes = bn_len;

  // Fail when sending bignums greater than 2^2^(8+16)
  bool success = true;
  log_assert(n_bytes < UINT16_MAX); // hard fail in debug
  if (n_bytes > UINT16_MAX) { // soft fail in release
    log_error("bignum exceeds maximum size for sending, 2^16 bytes");
    n_bytes = UINT16_MAX;
    success = false;
  }
  success = success && msg.template write<uint16_t>((uint16_t)n_bytes);

  uint8_t buf[bn_len];
  success = success && n_bytes == (size_t)BN_bn2bin(bn.peek(), buf);
  success = success && n_bytes == msg.add(buf, n_bytes);

  return success;
}

} // namespace ff
