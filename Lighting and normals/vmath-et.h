/*
    Quelques ajouts à vmath.h

    CC BY-SA Edouard.Thiel@univ-amu.fr - 30/01/2025
*/

#ifndef VMATH_ET_H
#define VMATH_ET_H

#include "vmath.h"

namespace vmath
{

    // Calcule le mineur 3x3 de la matrice 4x4
    mat3 minor3 (mat4& mat)
    {
        mat3 res;

        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++)
                res[j][i] = mat[j][i];

        return res;
    }


    // Calcule le déterminant de la matrice 3x3 avec la règle de Sarrus
    float det (mat3& mat)
    {
        return mat[0][0] * mat[1][1] * mat[2][2]
             + mat[1][0] * mat[2][1] * mat[0][2]
             + mat[2][0] * mat[0][1] * mat[1][2]
             - mat[0][2] * mat[1][1] * mat[2][0]
             - mat[1][2] * mat[2][1] * mat[0][0]
             - mat[2][2] * mat[0][1] * mat[1][0];
    }


    // Calcule l'inverse de la matrice 3x3 à partir de la comatrice
    mat3 inv (mat3& mat)
    {
        float d = det (mat);

        // Si la matrice est non inversible on renvoie la matice identité
        if (d == 0) return mat3::identity();

        // On calcule la comatrice
        mat3 res (
            vec3(
                mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1],
                mat[1][2]*mat[2][0] - mat[1][0]*mat[2][2],
                mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0]
            ),
            vec3(
                mat[0][2]*mat[2][1] - mat[0][1]*mat[2][2],
                mat[0][0]*mat[2][2] - mat[0][2]*mat[2][0],
                mat[0][1]*mat[2][0] - mat[0][0]*mat[2][1]
            ),
            vec3(
                mat[0][1]*mat[1][2] - mat[0][2]*mat[1][1],
                mat[0][2]*mat[1][0] - mat[0][0]*mat[1][2],
                mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0]
            )
        );

        // L'inverse est la transposée de la comatrice sur le déterminant
        return res.transpose() * (1.f / d);
    }


    // Calcule la matrice normale de la matrice 4x4, qui est la
    // transposée de l'inverse du mineur 3x3.
    mat3 normal (mat4& mat)
    {
        mat3 mat_min3 = minor3 (mat);
        mat3 mat_inv = inv (mat_min3);
        return mat_inv.transpose();
    }


    // Affiche une matrice 3x3
    void print (mat3& mat)
    {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++)
                std::cout << mat[j][i] << " ";
            std::cout << std::endl;
        }
    }

    // Affiche une matrice 4x4
    void print (mat4& mat)
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++)
                std::cout << mat[j][i] << " ";
            std::cout << std::endl;
        }
    }

};

#endif // VMATH_ET_H

