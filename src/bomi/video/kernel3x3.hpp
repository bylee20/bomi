//#ifndef KERNEL3X3_HPP
//#define KERNEL3X3_HPP

//class Kernel3x3 {
//public:
//    Kernel3x3() { mat(0, 0) = mat(2, 2) = 0.f; }
//    auto operator = (const Kernel3x3 &rhs) -> Kernel3x3&
//        { mat = rhs.mat; return *this; }
//    auto operator () (int i, int j) -> float& { return at(i, j); }
//    auto operator () (int i, int j) const -> float {return at(i, j); }
//    auto operator += (const Kernel3x3 &rhs) -> Kernel3x3&
//        { mat += rhs.mat; return *this; }
//    auto operator + (const Kernel3x3 &rhs) const -> Kernel3x3
//        { Kernel3x3 lhs = *this; return (lhs += rhs); }
//    auto operator == (const Kernel3x3 &rhs) const -> bool
//        { return mat == rhs.mat; }
//    auto operator != (const Kernel3x3 &rhs) const -> bool
//        { return !operator == (rhs); }
//    auto merged(const Kernel3x3 &other, bool normalize = true) -> Kernel3x3
//    {
//        Kernel3x3 ret = *this + other;
//        if (normalize)
//            ret.normalize();
//        return ret;
//    }
//    auto normalize() -> void
//    {
//        float den = 0.0;
//        for (int i=0; i<9; ++i)
//            den += mat.data()[i];
//        mat /= den;
//    }
//    auto normalized() const -> Kernel3x3
//    {
//        Kernel3x3 kernel = *this;
//        kernel.normalize();
//        return kernel;
//    }
//    float &at(int i, int j) { return mat(i, j); }
//    auto at(int i, int j) const -> float {return mat(i, j); }
//    auto setCenter(float v)-> void { mat(1, 1) = v; }
//    auto setNeighbor(float v)-> void
//        { mat(0, 1) = mat(1, 0) = mat(1, 2) = mat(2, 1) = v; }
//    auto setDiagonal(float v)-> void
//        { mat(0, 0) = mat(0, 2) = mat(2, 0) = mat(2, 2) = v;; }
//    auto center() const -> float {return mat(1, 1);}
//    auto neighbor() const -> float {return mat(0, 1);}
//    auto diagonal() const -> float {return mat(0, 0);}
//    auto matrix() const -> QMatrix3x3 {return mat;}
//private:
//    QMatrix3x3 mat;
//};

//#endif // KERNEL3X3_HPP
