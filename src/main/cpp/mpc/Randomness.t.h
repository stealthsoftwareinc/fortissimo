/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<typename Number_T>
Number_T randomModP(Number_T const & p) {
  Number_T try_rand = std::numeric_limits<Number_T>::max();

  do {
    if (!randomBytes(&try_rand, sizeof(Number_T))) {
      throw std::runtime_error("bad randomness");
    }
  } while (try_rand >= p * (std::numeric_limits<Number_T>::max() / p));
  return try_rand % p;
}

template<typename Number_T>
void arithmeticSecretShare(
    size_t const n_parties,
    Number_T const & p,
    Number_T const & num,
    ::std::vector<Number_T> & shares) {
  log_assert(num < p);
  shares.clear();
  shares.reserve(n_parties);

  for (size_t i = 1; i < n_parties; i++) {
    shares.push_back(randomModP(p));
  }

  log_assert(shares.size() == n_parties - 1);

  Number_T last = num;
  for (Number_T const & share : shares) {
    last = modSub(last, share, p);
  }
  shares.push_back(last);

  log_assert(shares.size() == n_parties);

  if_debug {
    Number_T check = 0;
    for (Number_T share : shares) {
      check = modAdd(check, share, p);
    }
    log_assert(num == check);
  }
}

template<typename Number_T>
void xorSecretShare(
    size_t const n_parties,
    Number_T const & num,
    ::std::vector<Number_T> & shares) {
  shares.clear();

  for (size_t i = 1; i < n_parties; i++) {
    Number_T r = 0;
    RAND_bytes((unsigned char *)&r, sizeof(Number_T));
    shares.push_back(r);
  }

  log_assert(shares.size() == n_parties - 1);

  Number_T last = num;
  for (Number_T const share : shares) {
    last = share ^ last;
  }
  shares.push_back(last);

  log_assert(shares.size() == n_parties);

  if_debug {
    Number_T check = 0;
    for (Number_T share : shares) {
      check = check ^ share;
    }
    log_assert(num == check);
  }
}

template<typename Rand_T, typename Info_T>
RandomnessDispenser<Rand_T, Info_T>::RandomnessDispenser(
    Info_T const & i) :
    info(i) {
}

template<typename Rand_T, typename Info_T>
void RandomnessDispenser<Rand_T, Info_T>::insert(
    Randomness_T const & val) {
  this->values.push_back(::std::move(val));
}

template<typename Rand_T, typename Info_T>
void RandomnessDispenser<Rand_T, Info_T>::insert(Randomness_T && val) {
  this->values.emplace_back(::std::move(val));
}

template<typename Rand_T, typename Info_T>
Rand_T RandomnessDispenser<Rand_T, Info_T>::get() {
  log_assert(this->values.size() > 0, "Out of randomness");
  Rand_T ret(std::move(this->values.front()));
  this->values.pop_front();
  return ret;
}

template<typename Rand_T, typename Info_T>
size_t RandomnessDispenser<Rand_T, Info_T>::size() {
  return this->values.size();
}

template<typename Rand_T, typename Info_T>
void RandomnessDispenser<Rand_T, Info_T>::shrink() {
  this->values.shrink_to_fit();
}

template<typename Rand_T, typename Info_T>
::std::unique_ptr<RandomnessDispenser<Rand_T, Info_T>>
RandomnessDispenser<Rand_T, Info_T>::littleDispenser(size_t const n) {
  if (n > this->size() || n == 0) {
    if (n != 0) {
      log_error("Not enough randomness");
    }

    return std::unique_ptr<
        RandomnessDispenser<Randomness_T, Information_T>>(nullptr);
  }

  std::unique_ptr<RandomnessDispenser<Randomness_T, Information_T>> ret(
      new RandomnessDispenser<Randomness_T, Information_T>(this->info));

  for (size_t i = 0; i < n; i++) {
    ret->insert(this->get());
  }

  return ret;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Rand_T,
    typename Info_T>
RandomnessGenerator<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T,
    Rand_T,
    Info_T>::RandomnessGenerator(size_t num, Info_T const & i) :
    numDesired(num), info(i) {
}
