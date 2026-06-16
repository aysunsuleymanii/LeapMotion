//
// Created by Aysun Suleymanturk on 2.3.26.
//

#pragma once

/**
 * @file Quaternion.h
 * @brief Quaternion type reserved for 3D orientation maths (HMD mode); not
 *        used by the current Desktop-mode gesture set.
 */
class Quaternion {
public:
    float x, y, z, w;

    Matrix3 toMatrix() const;
    Vector3 rotate(const Vector3& v) const;
};