/*************************************************************************/
/*  test_math.h                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef TEST_TRANSFORM_3D_H
#define TEST_TRANSFORM_3D_H

#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/transform_3d.h"
#include "core/os/os.h"

#include <math.h>
#include <stdio.h>

#include "tests/test_macros.h"

namespace TestTransform3D {

	TEST_CASE("[Transform3D] Rotate around global origin") {
		// Start with the default orientation, but not centered on the origin.
		// Rotating should rotate both our basis and the origin.
		Transform3D transform = Transform3D();
		transform.origin = Vector3(0, 0, 1);

		Transform3D expected = Transform3D();
		expected.origin = Vector3(0, 0, -1);
		expected.basis.set_axis(0, Vector3(-1, 0, 0));
		expected.basis.set_axis(2, Vector3(0, 0, -1));

		Transform3D rotated_transform = transform.rotated(Vector3(0, 1, 0), Math_PI);
		CHECK_MESSAGE(rotated_transform.is_equal_approx(expected), "The rotated transform should have a new orientation and basis.");
	}

	TEST_CASE("[Transform3D] Rotate in-place") {
		// Start with the default orientation, centered on the origin.
		// Rotating should rotate us around the origin, so the origin shouldn't change.
		Transform3D transform = Transform3D();

		Transform3D expected = Transform3D();
		expected.basis.set_axis(0, Vector3(-1, 0, 0));
		expected.basis.set_axis(2, Vector3(0, 0, -1));

		Transform3D rotated_transform = transform.rotated(Vector3(0, 1, 0), Math_PI);
		CHECK_MESSAGE(rotated_transform.is_equal_approx(expected), "The rotated transform should have a new orientation but still be based on the origin.");
	}

}

#endif
