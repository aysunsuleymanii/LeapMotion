//
// Created by Aysun Suleymanturk on 2.3.26.
//

#pragma once
class Quaternion {
public:
    float x, y, z, w;

    Matrix3 toMatrix() const;
    Vector3 rotate(const Vector3& v) const;
};