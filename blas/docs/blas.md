# BLAS {#blas}

`aest.blas` provides fixed-size vectors and matrices and the linear algebra
the other libraries build on, in the `aest::blas` namespace. Everything is
`constexpr` and avoids `<cmath>`, so matrices can be derived at compile time.

## Types

- aest::blas::Vector — `Vector<T, Size>`, a column vector: an alias of
  `std::array<T, Size>`.
- aest::blas::Matrix — `Matrix<T, Rows, Columns>`, an array of rows: an alias
  of `std::array<std::array<T, Columns>, Rows>`.

Being aliases of `std::array`, they are aggregates: build them with braces,
compare them with `==`, and pass them to any code taking `std::array`.

```cpp
constexpr Matrix<double, 3, 3> scale = {{{2, 0, 0}, {0, 4, 0}, {0, 0, 8}}};
```

## Operations

| Function | Does | Sizes |
|----------|------|-------|
| aest::blas::multiply() | Matrix times column vector | Any `Rows` × `Columns` |
| aest::blas::determinant() | Determinant, by cofactor expansion | 3×3 |
| aest::blas::inverse() | Inverse, through the adjugate; floating-point only | 3×3 |

The functions are free templates in `aest::blas`. The types are aliases of
`std::array`, so argument-dependent lookup does not find them: qualify the
calls, or bring them in with `using`.
