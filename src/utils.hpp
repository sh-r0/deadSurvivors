#pragma once
#include <algorithm>
#include <cmath>
#include "gameTypes.hpp"

template<typename T> 
inline T abs(T _x) {
    return _x < 0 ? -_x : _x;
}

template<typename T>
void normalizeVec(std::array<T, 2>& _vec) {
	double vecLength = sqrt(_vec[0] * _vec[0] + _vec[1] * _vec[1]);
	_vec[0] = _vec[0] / vecLength;
	_vec[1] = _vec[1] / vecLength;

	return;
}
