/**************************************************************************/
/*  test_quad_occluder_3d.h                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/*************************************************************s*************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/
#ifndef TEST_QUAD_OCCLUDER_3D_H
#define TEST_QUAD_OCCLUDER_3D_H

#include "scene/3d/occluder_instance_3d.h"
#include "core/math/vector3.h"

#include "tests/test_macros.h"
namespace QuadOccluder3DTest {

TEST_CASE("[SceneTree][Occluder3D] QuadOccluder3d") {
    // Define expected shape
    PackedInt32Array expected_indices = PackedInt32Array({
        0, 1, 2, 0, 2, 3
    });
    PackedVector3Array expected_vertices = PackedVector3Array(
        {Vector3(-1.0, -1.0, 0.0), Vector3(-1.0, 1.0, 0.0), Vector3(1.0, 1.0, 0.0), Vector3(1.0, -1.0, 0.0)}
    );

    // Create QuadOcculuder
    QuadOccluder3D *TestQuad = memnew(QuadOccluder3D);
    TestQuad->set_size(Vector2(2,2));

    // Get returned indices and vertices
    PackedInt32Array returned_indices = TestQuad->get_indices();
    PackedVector3Array returned_vertices = TestQuad->get_vertices();

    // Check with expected result
    CHECK_EQ(returned_indices,expected_indices);
    CHECK_EQ(returned_vertices,expected_vertices);

    memdelete(TestQuad);
}

} // namespace QuadOccluder3DTest
#endif 