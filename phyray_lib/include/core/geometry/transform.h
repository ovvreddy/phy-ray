#ifndef PHYRAY_CORE_TRANSFORM_H
#define PHYRAY_CORE_TRANSFORM_H

#include <core/phyr.h>
#include <core/phyr_math.h>
#include <core/geometry/geometry.h>

namespace phyr {

// Transform class declarations
class Transform {
  public:
    // Constructors
    Transform() {}
    Transform(const Real tMat[4][4]) {
        mat = Mat4x4(tMat[0][0], tMat[0][1], tMat[0][2], tMat[0][3],
                     tMat[1][0], tMat[1][1], tMat[1][2], tMat[1][3],
                     tMat[2][0], tMat[2][1], tMat[2][2], tMat[2][3],
                     tMat[3][0], tMat[3][1], tMat[3][2], tMat[3][3]);
        invMat = Mat4x4::inverse(mat);
    }
    Transform(const Mat4x4& mat) : mat(mat), invMat(Mat4x4::inverse(mat)) {}
    Transform(const Mat4x4& mat, const Mat4x4& invMat) : mat(mat), invMat(invMat) {}

    // Getters
    const Mat4x4& getMatrix() const { return mat; }
    const Mat4x4& getInverseMatrix() const { return invMat; }

    // Other public functions
    inline bool operator==(const Transform& t) const {
        return mat == t.mat && invMat == t.invMat;
    }
    inline bool operator!=(const Transform& t) const {
        return mat != t.mat && invMat != t.invMat;
    }
    bool isIdentity() const { return mat == Mat4x4(); }

    /**
     * Returns true if this transform changes the
     * destination coordinate system.
     *
     * This is only true if the determinant of the upper left
     * 3x3 matrix is negative
     */
    bool changesCoordinateSystem() const {
        Real det = mat.d[0][0] * (mat.d[1][1] * mat.d[2][2] - mat.d[1][2] * mat.d[2][1]) -
                   mat.d[0][1] * (mat.d[1][0] * mat.d[2][2] - mat.d[1][2] * mat.d[2][0]) +
                   mat.d[0][2] * (mat.d[1][0] * mat.d[2][1] - mat.d[1][1] * mat.d[2][0]);
        return det < 0;
    }
    Transform operator*(const Transform& t);

    // Transforms are applied on geometric primitives through
    // the overloaded function operator for each primitive type
    template <typename T>
    inline Point3<T> operator()(const Point3<T>& p) const;
    template <typename T>
    inline Vector3<T> operator()(const Vector3<T>& v) const;
    template <typename T>
    inline Normal3<T> operator()(const Normal3<T>& n) const;

    inline Ray operator()(const Ray& r) const;
    inline Bounds3f operator()(const Bounds3f& b) const;

    // Static functions
    static Transform inverse(const Transform& t) {
        return Transform(t.invMat, t.mat);
    }
    static Transform transpose(const Transform& t) {
        return Transform(Mat4x4::transpose(t.mat), Mat4x4::transpose(t.invMat));
    }

    // Factory methods
    static Transform translate(const Vector3f& v);
    static Transform scale(Real sx, Real sy, Real sz);

    // Axis aligned rotation
    static Transform rotateX(Real theta);
    static Transform rotateY(Real theta);
    static Transform rotateZ(Real theta);
    // Arbitrary axis rotation
    static Transform rotate(Real theta, const Vector3f& axis);

    // Camera lookAt
    static Transform lookAt(const Point3f& pos, const Point3f& lookPos, const Vector3f& up);

  private:
    Mat4x4 mat, invMat;
};

} // namespace phyr

#endif
