/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<typename Number_T>
Matrix<Number_T>::Matrix(
    std::vector<Number_T> && buf, size_t const nr, size_t const nc) :
    buffer(::std::move(buf)), numRows(nr), numColumns(nc) {
  log_assert(this->buffer.size() == this->numColumns * this->numRows);
}

template<typename Number_T>
Matrix<Number_T>::Matrix(
    std::vector<Number_T> const & buf,
    size_t const nr,
    size_t const nc) :
    buffer(buf), numRows(nr), numColumns(nc) {
  log_assert(this->buffer.size() == this->numColumns * this->numRows);
}

template<typename Number_T>
Matrix<Number_T>::Matrix(size_t const nr, size_t const nc) :
    buffer(nc * nr, static_cast<Number_T>(0)),
    numRows(nr),
    numColumns(nc) {
  log_assert(this->buffer.size() == this->numColumns * this->numRows);
}

template<typename Number_T>
Matrix<Number_T>::Matrix(size_t const id) : Matrix<Number_T>(id, id) {
  for (size_t i = 0; i < this->numRows; i++) {
    this->at(i, i) = 1;
  }
}

template<typename Number_T>
std::string Matrix<Number_T>::Print() const {
  std::string to_return = "(";
  for (size_t i = 0; i < this->numRows; i++) {
    for (size_t j = 0; j < this->numColumns; j++) {
      if (j > 0)
        to_return += "\t";
      to_return += ff::mpc::dec(this->at(i, j));
    }
    if (i < this->numRows - 1)
      to_return += "\n";
  }
  to_return += ")\n";
  return to_return;
}

template<typename Number_T>
size_t Matrix<Number_T>::getNumRows() const {
  return this->numRows;
}

template<typename Number_T>
size_t Matrix<Number_T>::getNumColumns() const {
  return this->numColumns;
}

template<typename Number_T>
Number_T & Matrix<Number_T>::at(size_t const i, size_t const j) {
  log_assert(i < this->numRows);
  log_assert(j < this->numColumns);
  return this->buffer[(this->numColumns * i) + j];
}

template<typename Number_T>
Number_T const &
Matrix<Number_T>::at(size_t const i, size_t const j) const {
  log_assert(i < this->numRows);
  log_assert(j < this->numColumns);
  return this->buffer[(this->numColumns * i) + j];
}

template<typename Number_T>
void Matrix<Number_T>::MakeIdentity(
    const Number_T & modulus, Matrix<Number_T> & b) const {
  Matrix<Number_T> Mcopy(*this);

  for (size_t i = 0; i < this->numRows - 1; ++i) {
    // Find the first row that has a non-zero entry in column i.
    size_t i2 = i;
    while (i2 < this->numRows && Mcopy.at(i2, i) == 0) {
      i2++;
    }

    log_assert(i2 != this->numRows);

    if (i2 != i) {
      // Swap row i and row i2.

      for (size_t k = i; k < Mcopy.numColumns; ++k) {
        Number_T tmp = Mcopy.at(i, k);
        Mcopy.at(i, k) = Mcopy.at(i2, k);
        Mcopy.at(i2, k) = tmp;
      }

      // Ditto, for augmented matrix.
      for (size_t k = 0; k < b.numColumns; ++k) {
        Number_T tmp = b.at(i, k);
        b.at(i, k) = b.at(i2, k);
        b.at(i2, k) = tmp;
      }
    }

    // Divide row i by Mcopy.at(i,i).
    Number_T inv = modInvert<Number_T>(Mcopy.at(i, i), modulus);
    for (size_t k = i; k < Mcopy.numColumns; ++k) {
      Mcopy.at(i, k) = modMul<Number_T>(Mcopy.at(i, k), inv, modulus);
    }
    // Ditto, for augmented matrix.
    for (size_t k = 0; k < b.numColumns; ++k) {
      b.at(i, k) = modMul<Number_T>(b.at(i, k), inv, modulus);
    }

    // Subtract row i from rows i+1...(numRows-1)
    for (size_t i2 = i + 1; i2 < Mcopy.numRows; ++i2) {
      Number_T tmp = Mcopy.at(i2, i);
      for (size_t l = i; l < Mcopy.numColumns; ++l) {
        Number_T ratio = modMul<Number_T>(tmp, Mcopy.at(i, l), modulus);
        ratio = modulus - ratio;
        Mcopy.at(i2, l) =
            modAdd<Number_T>(Mcopy.at(i2, l), ratio, modulus);
      }
      // Ditto, for augmented matrix.
      for (size_t l = 0; l < b.numColumns; ++l) {
        Number_T ratio = modMul<Number_T>(tmp, b.at(i, l), modulus);
        ratio = modulus - ratio;
        b.at(i2, l) = modAdd<Number_T>(b.at(i2, l), ratio, modulus);
      }
    }
  }

  // Done with all rows except last; the last row just has a non-identity
  // element in the last column (i.e. the on-diagonal position).
  Number_T inv = modInvert<Number_T>(
      Mcopy.at(Mcopy.numRows - 1, Mcopy.numColumns - 1), modulus);
  Mcopy.at(Mcopy.numRows - 1, Mcopy.numColumns - 1) = 1;
  for (size_t l = 0; l < b.numColumns; ++l) {
    b.at(Mcopy.numRows - 1, l) =
        modMul<Number_T>(b.at(Mcopy.numRows - 1, l), inv, modulus);
  }

  // Now cancel all the upper-triangle coordinates.
  for (size_t i = Mcopy.numRows - 1; i > 0; --i) {
    // Subtract row i from rows i-1...0.
    for (size_t l = 0; l < i; ++l) {
      Number_T tmp = Mcopy.at(l, i);
      for (size_t k = 0; k < b.numColumns; ++k) {
        Number_T ratio = modMul<Number_T>(b.at(i, k), tmp, modulus);
        ratio = modulus - ratio;
        b.at(l, k) = modAdd<Number_T>(b.at(l, k), ratio, modulus);
      }
    }
  }
}

template<typename Number_T>
Matrix<Number_T>
Matrix<Number_T>::Inverse(const Number_T & modulus) const {
  Matrix<Number_T> to_return = *this;
  to_return.Invert(modulus);
  return to_return;
}

template<typename Number_T>
void Matrix<Number_T>::Invert(const Number_T & modulus) {
  log_assert(this->numRows > 0 && this->numRows == this->numColumns);
  // Make the augmented Identity matrix.
  Matrix<Number_T> b(this->numRows);

  // Transform M into Identity (without actually changing M), and make the
  // corresponding changes to b = Identity.
  MakeIdentity(modulus, b);

  // Inverse is now stored in 'b'. Swap buffers.
  this->buffer = std::move(b.buffer);
}

template<typename Number_T>
Matrix<Number_T> Matrix<Number_T>::Transpose() const {
  Matrix<Number_T> to_return = *this;
  to_return.DoTranspose();
  return to_return;
}

// In-place transpose is difficult: code is complicated (most efficient algorithm
// is to "swap in a cycle", and then move from one cycle to the next):
//   https://en.wikipedia.org/wiki/In-place_matrix_transposition
// Instead, we use a more memory intensive approach (non-in-place), and then
// do a std::move() at the end.
template<typename Number_T>
void Matrix<Number_T>::DoTranspose() {
  // Create temporary storage.
  std::vector<Number_T> tmp(this->buffer.size());
  for (size_t index = 0; index < this->buffer.size(); ++index) {
    const size_t i = index / this->numRows;
    const size_t j = index % this->numColumns;
    if (i == j)
      continue;
    // Move (i, j) to (j, i). Notice that in the new arrangement,
    // index (j, i) actually has index: j * numColumns + i.
    tmp[j * this->numColumns + i] = this->at(i, j);
  }

  // Copy matrix to 'buffer'.
  this->buffer = std::move(tmp);

  // Update new dimensions of matrix.
  size_t old_num_rows = this->numRows;
  this->numRows = this->numColumns;
  this->numColumns = old_num_rows;
}

template<typename Number_T>
Number_T Matrix<Number_T>::Det(const Number_T & modulus) const {
  log_assert(this->numRows > 0 && this->numRows == this->numColumns);
  // Make a copy, since this method is const and the Determinant
  // protocol needs to modify the matrix.
  Matrix<Number_T> tmp = *this;
  bool neg_one = false;
  Number_T to_return = 1;
  for (size_t i = 0; i < tmp.numRows - 1; ++i) {
    // Find the first row that has a non-zero entry in column i.
    size_t i2 = i;
    while (i2 < tmp.numRows && tmp.at(i2, i) == 0) {
      i2++;
    }
    if (i2 == tmp.numRows) {
      return 0;
    }
    if (i2 != i) {
      // Swap row i and row i2.
      neg_one ^=
          true; // Determinant switches sign when swapping two rows.
      for (size_t k = i; k < tmp.numColumns; ++k) {
        Number_T tmp2 = tmp.at(i, k);
        tmp.at(i, k) = tmp.at(i2, k);
        tmp.at(i2, k) = tmp2;
      }
    }

    // Divide row i by tmp.at(i,i).
    Number_T inv = modInvert<Number_T>(tmp.at(i, i), modulus);
    // Multiplying a row by 'inv' will affect the determinant
    // by a factor 1 / inv (i.e. by original value).
    to_return = modMul<Number_T>(to_return, tmp.at(i, i), modulus);
    for (size_t k = i; k < tmp.numColumns; ++k) {
      tmp.at(i, k) = modMul<Number_T>(tmp.at(i, k), inv, modulus);
    }

    // Subtract row i from rows i+1...(numRows-1)
    for (size_t i2 = i + 1; i2 < tmp.numRows; ++i2) {
      Number_T tmp2 = tmp.at(i2, i);
      for (size_t l = i; l < tmp.numColumns; ++l) {
        Number_T ratio = modMul<Number_T>(tmp2, tmp.at(i, l), modulus);
        ratio = modulus - ratio;
        tmp.at(i2, l) = modAdd<Number_T>(tmp.at(i2, l), ratio, modulus);
      }
    }
  }

  // Done with all rows except last; the last row just has a non-identity
  // element in the last column (i.e. the on-diagonal position).
  to_return = modMul<Number_T>(
      to_return, tmp.at(tmp.numRows - 1, tmp.numRows - 1), modulus);

  // Finally, apply -1 factor, if relevant.
  if (neg_one && to_return != 0)
    to_return = modulus - to_return;
  return to_return;
}

template<typename Number_T>
Number_T Matrix<Number_T>::Trace() const {
  if (this->numRows != this->numColumns)
    return 0;
  Number_T to_return = 0;
  for (size_t i = 0; i < this->numRows; ++i) {
    to_return += this->at(i, i);
  }
  return to_return;
}

template<typename Number_T>
void plainMatrixMult(
    Matrix<Number_T> const * const A,
    Matrix<Number_T> const * const B,
    Matrix<Number_T> * const C,
    Number_T const p) {
  log_assert(A->getNumColumns() == B->getNumRows());
  log_assert(C->getNumRows() == A->getNumRows());
  log_assert(C->getNumColumns() == B->getNumColumns());

  for (size_t i = 0; i < A->getNumRows(); i++) {
    for (size_t j = 0; j < B->getNumColumns(); j++) {
      Number_T sum = 0;
      for (size_t k = 0;
           k < A->getNumColumns() /* && k < B->getNumRows() */;
           k++) {
        Number_T const a = A->at(i, k);
        Number_T const b = B->at(k, j);
        sum = modAdd<Number_T>(sum, a * b, p);
      }
      C->at(i, j) = sum;
    }
  }
}

} // namespace mpc
} // namespace ff
