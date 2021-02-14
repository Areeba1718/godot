/*************************************************************************/
/*  space_3d_sw.cpp                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "space_3d_sw.h"

#include "collision_solver_3d_sw.h"
#include "physics_server_3d_sw.h"

_FORCE_INLINE_ static bool _can_collide_with(const CollisionObject3DSW *p_object, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas) {
	if (!(p_object->get_collision_layer() & p_collision_mask)) {
		return false;
	}

	if (p_object->get_type() == CollisionObject3DSW::TYPE_AREA && !p_collide_with_areas) {
		return false;
	}

	if (p_object->get_type() == CollisionObject3DSW::TYPE_BODY && !p_collide_with_bodies) {
		return false;
	}

	return true;
}

int PhysicsDirectSpaceState3DSW::intersect_point(const Vector3 &p_point, ShapeResult *r_results, int p_result_max, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas) {
	ERR_FAIL_COND_V_MSG(space->locked, 0, "Space is locked");

	int pair_count = space->broadphase->cull_point(p_point, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	int col_count = 0;

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		if (col_count >= p_result_max) {
			break;
		}

		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_inv_xform = col_obj->get_shape_inv_transform(col_shape_idx) * col_obj->get_inv_transform();
		Vector3 local_point = col_shape_inv_xform.xform(p_point);

		if (!col_shape->intersect_point(local_point)) {
			continue;
		}

		r_results[col_count].rid = col_obj->get_self();
		r_results[col_count].collider_id = col_obj->get_instance_id();
		if (r_results[col_count].collider_id.is_valid()) {
			r_results[col_count].collider = ObjectDB::get_instance(r_results[col_count].collider_id);
		}
		r_results[col_count].shape = col_shape_idx;

		col_count++;
	}

	return col_count;
}

bool PhysicsDirectSpaceState3DSW::intersect_ray(const Vector3 &p_from, const Vector3 &p_to, RayResult &r_result, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, bool p_pick_ray) {
	ERR_FAIL_COND_V_MSG(space->locked, false, "Space is locked");

	Vector3 normal = (p_to - p_from).normalized();

	int pair_count = space->broadphase->cull_segment(p_from, p_to, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	//TODO Create another array that references results, compute AABBs and check closest point to ray origin, sort, and stop evaluating results when beyond first collision

	bool collided = false;
	real_t min_distance = 10e20;

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check ray pickable exclusions
		if (p_pick_ray && !(col_obj->is_ray_pickable())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_inv_xform = col_obj->get_shape_inv_transform(col_shape_idx) * col_obj->get_inv_transform();

		Vector3 local_from = col_shape_inv_xform.xform(p_from);
		Vector3 local_to = col_shape_inv_xform.xform(p_to);
		Vector3 shape_point, shape_normal;

		if (!col_shape->intersect_segment(local_from, local_to, shape_point, shape_normal)) {
			continue;
		}

		const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);
		shape_point = col_shape_xform.xform(shape_point);

		real_t distance = normal.dot(shape_point);

		if (distance < min_distance) {
			collided = true;
			min_distance = distance;

			r_result.position = shape_point;
			r_result.normal = col_shape_inv_xform.basis.xform_inv(shape_normal).normalized();
			r_result.rid = col_obj->get_self();
			r_result.collider_id = col_obj->get_instance_id();
			if (r_result.collider_id.is_valid()) {
				r_result.collider = ObjectDB::get_instance(r_result.collider_id);
			}
			r_result.shape = col_shape_idx;
		}
	}

	return collided;
}

int PhysicsDirectSpaceState3DSW::intersect_shape(const RID &p_shape, const Transform &p_shape_xform, real_t p_margin, ShapeResult *r_results, int p_result_max, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas) {
	ERR_FAIL_COND_V_MSG(space->locked, 0, "Space is locked");

	const Shape3DSW *shape = PhysicsServer3DSW::singletonsw->shape_owner.getornull(p_shape);
	ERR_FAIL_COND_V_MSG(!shape, 0, "RID is not a shape");

	AABB aabb = p_shape_xform.xform(shape->get_aabb());
	aabb = aabb.grow(p_margin);

	int pair_count = space->broadphase->cull_aabb(aabb, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	int col_count = 0;

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		if (col_count >= p_result_max) {
			break;
		}

		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

		if (!CollisionSolver3DSW::solve_static(shape, p_shape_xform, col_shape, col_shape_xform, nullptr, nullptr, nullptr, p_margin)) {
			continue;
		}

		r_results[col_count].rid = col_obj->get_self();
		r_results[col_count].collider_id = col_obj->get_instance_id();
		if (r_results[col_count].collider_id.is_valid()) {
			r_results[col_count].collider = ObjectDB::get_instance(r_results[col_count].collider_id);
		}
		r_results[col_count].shape = col_shape_idx;

		col_count++;
	}

	return col_count;
}

bool PhysicsDirectSpaceState3DSW::cast_motion(const RID &p_shape, const Transform &p_shape_xform, const Vector3 &p_motion, real_t p_margin, real_t &r_best_safe, real_t &r_best_unsafe, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, ShapeRestInfo *r_info) {
	r_best_safe = 0;
	r_best_unsafe = 0;

	ERR_FAIL_COND_V_MSG(space->locked, false, "Space is locked");

	if (p_motion == Vector3()) {
		return false;
	}

	Shape3DSW *shape = PhysicsServer3DSW::singletonsw->shape_owner.getornull(p_shape);
	ERR_FAIL_COND_V_MSG(!shape, false, "RID is not a shape");

	AABB aabb = p_shape_xform.xform(shape->get_aabb());
	aabb = aabb.merge(AABB(aabb.position + p_motion, aabb.size));
	aabb = aabb.grow(p_margin);
	Vector3 motion_normal = p_motion.normalized();

	int pair_count = space->broadphase->cull_aabb(aabb, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	const Transform xform_inv = p_shape_xform.affine_inverse();
	MotionShape3DSW mshape;
	mshape.shape = shape;
	mshape.motion = xform_inv.basis.xform(p_motion);

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

		// Does it collide if going all the way?
		Vector3 point_A, point_B;
		Vector3 sep_axis = motion_normal;
		if (CollisionSolver3DSW::solve_distance(&mshape, p_shape_xform, col_shape, col_shape_xform, point_A, point_B, aabb, &sep_axis)) {
			continue;
		}

		// Ignore objects it's inside of.
		sep_axis = motion_normal;
		if (!CollisionSolver3DSW::solve_distance(shape, p_shape_xform, col_shape, col_shape_xform, point_A, point_B, aabb, &sep_axis)) {
			continue;
		}

		// Do kinematic solving
		real_t low = 0.0;
		real_t hi = 1.0;
		Vector3 closest_A, closest_B;

		for (int step = 0; step < 8; step++) { // Steps should be customizable.
			real_t ofs = (low + hi) * 0.5;

			sep_axis = motion_normal; // Important optimization for this to work fast enough.
			mshape.motion = xform_inv.basis.xform(p_motion * ofs);
			if (CollisionSolver3DSW::solve_distance(&mshape, p_shape_xform, col_shape, col_shape_xform, point_A, point_B, aabb, &sep_axis)) {
				low = ofs;
				closest_A = point_A;
				closest_B = point_B;
			} else {
				hi = ofs;
			}
		}

		if (low < r_best_safe) {
			r_best_safe = low;
			r_best_unsafe = hi;

			if (r_info) {
				r_info->point = closest_B;
				r_info->normal = (closest_A - closest_B).normalized();
				r_info->rid = col_obj->get_self();
				r_info->collider_id = col_obj->get_instance_id();
				r_info->shape = col_shape_idx;
				if (col_obj->get_type() == CollisionObject3DSW::TYPE_BODY) {
					const Body3DSW *body = static_cast<const Body3DSW *>(col_obj);
					r_info->linear_velocity = body->get_linear_velocity() + (body->get_angular_velocity()).cross(body->get_transform().origin - closest_B);
				}
			}
		}
	}

	return true;
}

bool PhysicsDirectSpaceState3DSW::collide_shape(RID p_shape, const Transform &p_shape_xform, real_t p_margin, Vector3 *r_results, int p_result_max, int &r_result_count, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas) {
	ERR_FAIL_COND_V_MSG(space->locked, false, "Space is locked");

	const Shape3DSW *shape = PhysicsServer3DSW::singletonsw->shape_owner.getornull(p_shape);
	ERR_FAIL_COND_V_MSG(!shape, false, "RID is not a shape");

	AABB aabb = shape->get_aabb();
	aabb = p_shape_xform.xform(aabb);
	aabb = aabb.grow(p_margin);

	int pair_count = space->broadphase->cull_aabb(aabb, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	CollisionSolver3DSW::CallbackResult col_cbk = PhysicsServer3DSW::_shape_col_cbk;
	PhysicsServer3DSW::CollCbkData col_cbk_data;
	col_cbk_data.max = p_result_max;
	col_cbk_data.amount = 0;
	col_cbk_data.ptr = r_results;

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

		CollisionSolver3DSW::solve_static(shape, p_shape_xform, col_shape, col_shape_xform, col_cbk, &col_cbk_data, nullptr, p_margin);
	}

	r_result_count = col_cbk_data.amount;

	return r_result_count > 0;
}

struct _RestCallbackData {
	const CollisionObject3DSW *object = nullptr;
	const CollisionObject3DSW *best_object = nullptr;
	int shape = 0;
	int best_shape = 0;
	Vector3 best_contact;
	Vector3 best_normal;
	real_t best_len = 0;
	real_t min_allowed_depth = 0;
};

static void _rest_cbk_result(const Vector3 &p_point_A, const Vector3 &p_point_B, void *p_userdata) {
	_RestCallbackData *rd = (_RestCallbackData *)p_userdata;

	Vector3 contact_rel = p_point_B - p_point_A;
	real_t len = contact_rel.length();

	if (len < rd->min_allowed_depth || len <= rd->best_len) {
		return;
	}

	rd->best_len = len;
	rd->best_contact = p_point_B;
	rd->best_normal = contact_rel / len;
	rd->best_object = rd->object;
	rd->best_shape = rd->shape;
}

bool PhysicsDirectSpaceState3DSW::rest_info(RID p_shape, const Transform &p_shape_xform, real_t p_margin, ShapeRestInfo *r_info, const Set<RID> &p_exclude, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas) {
	ERR_FAIL_COND_V_MSG(space->locked, false, "Space is locked");

	const Shape3DSW *shape = PhysicsServer3DSW::singletonsw->shape_owner.getornull(p_shape);
	ERR_FAIL_COND_V_MSG(!shape, false, "RID is not a shape");

	AABB aabb = shape->get_aabb();
	aabb = p_shape_xform.xform(aabb);
	aabb = aabb.grow(p_margin);

	int pair_count = space->broadphase->cull_aabb(aabb, space->intersection_query_results, Space3DSW::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	_RestCallbackData rcd;
	rcd.min_allowed_depth = space->test_motion_min_contact_depth;

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		const CollisionObject3DSW *col_obj = space->intersection_query_results[pair_idx];

		// Check exclusions
		if (p_exclude.has(col_obj->get_self())) {
			continue;
		}

		// Check collision filters
		if (!_can_collide_with(col_obj, p_collision_mask, p_collide_with_bodies, p_collide_with_areas)) {
			continue;
		}

		int col_shape_idx = space->intersection_query_subindex_results[pair_idx];
		const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
		const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

		rcd.object = col_obj;
		rcd.shape = col_shape_idx;

		CollisionSolver3DSW::solve_static(shape, p_shape_xform, col_shape, col_shape_xform, _rest_cbk_result, &rcd, nullptr, p_margin);
	}

	if (rcd.best_len == 0 || !rcd.best_object) {
		return false;
	}

	r_info->point = rcd.best_contact;
	r_info->normal = rcd.best_normal;
	r_info->rid = rcd.best_object->get_self();
	r_info->collider_id = rcd.best_object->get_instance_id();
	r_info->shape = rcd.best_shape;
	if (rcd.best_object->get_type() == CollisionObject3DSW::TYPE_BODY) {
		const Body3DSW *body = static_cast<const Body3DSW *>(rcd.best_object);
		r_info->linear_velocity = body->get_linear_velocity() + (body->get_angular_velocity()).cross(body->get_transform().origin - rcd.best_contact);
	}

	return true;
}

Vector3 PhysicsDirectSpaceState3DSW::get_closest_point_to_object_volume(RID p_object, const Vector3 p_point) const {
	ERR_FAIL_COND_V_MSG(space->locked, Vector3(), "Space is locked");

	CollisionObject3DSW *obj = PhysicsServer3DSW::singletonsw->area_owner.getornull(p_object);
	ERR_FAIL_COND_V_MSG(!obj, Vector3(), "RID is not an object");
	ERR_FAIL_COND_V_MSG(obj->get_space() != space, Vector3(), "Object not in this space");

	real_t min_distance = 1e20;
	Vector3 min_point;
	bool shapes_found = false;

	for (int shape_idx = 0; shape_idx < obj->get_shape_count(); shape_idx++) {
		if (obj->is_shape_set_as_disabled(shape_idx)) {
			continue;
		}

		const Transform shape_xform = obj->get_transform() * obj->get_shape_transform(shape_idx);
		const Shape3DSW *shape = obj->get_shape(shape_idx);

		Vector3 point = shape->get_closest_point_to(shape_xform.affine_inverse().xform(p_point));
		point = shape_xform.xform(point);

		real_t distance = point.distance_to(p_point);
		if (distance < min_distance) {
			min_distance = distance;
			min_point = point;
		}

		shapes_found = true;
	}

	if (!shapes_found) {
		return obj->get_transform().origin; // No shapes found, use distance to origin.
	}

	return min_point;
}

PhysicsDirectSpaceState3DSW::PhysicsDirectSpaceState3DSW() {
	space = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Space3DSW::_cull_aabb_for_body(Body3DSW *p_body, const AABB &p_aabb) {
	int pair_count = broadphase->cull_aabb(p_aabb, intersection_query_results, INTERSECTION_QUERY_MAX, intersection_query_subindex_results);

	for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
		bool keep = true;

		const CollisionObject3DSW *col_obj = intersection_query_results[pair_idx];
		if (col_obj == p_body)
			keep = false;
		else if (col_obj->get_type() == CollisionObject3DSW::TYPE_AREA)
			keep = false;
		else if (col_obj->is_shape_set_as_disabled(intersection_query_subindex_results[pair_idx]))
			keep = false;
		else if (!col_obj->test_collision_mask(p_body))
			keep = false;
		else if (static_cast<const Body3DSW *>(col_obj)->has_exception(p_body->get_self()) || p_body->has_exception(col_obj->get_self()))
			keep = false;

		if (!keep) {
			if (pair_idx < pair_count - 1) {
				SWAP(intersection_query_results[pair_idx], intersection_query_results[pair_count - 1]);
				SWAP(intersection_query_subindex_results[pair_idx], intersection_query_subindex_results[pair_count - 1]);
			}
			pair_count--;
			pair_idx--;
		}
	}

	return pair_count;
}

int Space3DSW::test_body_ray_separation(Body3DSW *p_body, const Transform &p_xform, bool p_infinite_inertia, Vector3 &r_recover_motion, PhysicsServer3D::SeparationResult *r_results, int p_result_max, real_t p_margin) {
	AABB body_aabb;
	bool ray_shapes_found = false;

	for (int body_shape_idx = 0; body_shape_idx < p_body->get_shape_count(); body_shape_idx++) {
		if (p_body->is_shape_set_as_disabled(body_shape_idx)) {
			continue;
		}

		// Ignore all shapes except ray shapes
		if (p_body->get_shape(body_shape_idx)->get_type() != PhysicsServer3D::SHAPE_RAY) {
			continue;
		}

		if (!ray_shapes_found) {
			body_aabb = p_body->get_shape_aabb(body_shape_idx);
			ray_shapes_found = true;
		} else {
			body_aabb = body_aabb.merge(p_body->get_shape_aabb(body_shape_idx));
		}
	}

	if (!ray_shapes_found) {
		return 0;
	}

	// Undo the currently transform the physics server is aware of and apply the provided one
	body_aabb = p_xform.xform(p_body->get_inv_transform().xform(body_aabb));
	body_aabb = body_aabb.grow(p_margin);

	Transform body_transform = p_xform;

	// Initialise collision depths
	for (int i = 0; i < p_result_max; i++) {
		r_results[i].collision_depth = 0;
	}

	int rays_found = 0;

	// Raycast and separate
	CollisionSolver3DSW::CallbackResult col_cbk = PhysicsServer3DSW::_shape_col_cbk;
	PhysicsServer3DSW::CollCbkData col_cbk_data;
	Vector3 col_points[2];
	col_cbk_data.ptr = col_points;
	col_cbk_data.max = 1;

	for (int recover_attempt = 0; recover_attempt < 4; recover_attempt++) {
		Vector3 recover_motion;

		int pair_count = _cull_aabb_for_body(p_body, body_aabb);

		for (int body_shape_idx = 0; body_shape_idx < p_body->get_shape_count(); body_shape_idx++) {
			if (p_body->is_shape_set_as_disabled(body_shape_idx)) {
				continue;
			}

			const Shape3DSW *body_shape = p_body->get_shape(body_shape_idx);

			// Ignore all shapes except ray shapes
			if (body_shape->get_type() != PhysicsServer3D::SHAPE_RAY) {
				continue;
			}

			const Transform body_shape_xform = body_transform * p_body->get_shape_transform(body_shape_idx);

			int ray_idx = rays_found;
			// If ray already has a result, reuse it.
			for (int result_idx = 0; result_idx < rays_found && result_idx < p_result_max; result_idx++) {
				if (r_results[result_idx].collision_local_shape == body_shape_idx) {
					ray_idx = result_idx;
				}
			}

			for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
				// Reset counter
				col_cbk_data.amount = 0;

				const CollisionObject3DSW *col_obj = intersection_query_results[pair_idx];

				// Ignore RigidBodies if infinite inertia is enabled
				if (CollisionObject3DSW::TYPE_BODY == col_obj->get_type()) {
					const Body3DSW *b = static_cast<const Body3DSW *>(col_obj);
					if (p_infinite_inertia && b->get_mode() != PhysicsServer3D::BODY_MODE_STATIC && b->get_mode() != PhysicsServer3D::BODY_MODE_KINEMATIC) {
						continue;
					}
				}

				int col_shape_idx = intersection_query_subindex_results[pair_idx];
				const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
				const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

				if (CollisionSolver3DSW::solve_static(body_shape, body_shape_xform, col_shape, col_shape_xform, col_cbk, &col_cbk_data, nullptr, p_margin)) {
					Vector3 a = col_points[0];
					Vector3 b = col_points[1];
					recover_motion += (b - a) / col_cbk_data.amount;

					// Check whether this is the ray's first result.
					if (ray_idx == rays_found) {
						rays_found++;
					}

					// Check whether or not the max results has been reached.
					if (ray_idx >= p_result_max) {
						continue;
					}

					PhysicsServer3D::SeparationResult &result = r_results[ray_idx];
					real_t depth = a.distance_to(b);

					if (depth > result.collision_depth) {
						result.collision_depth = depth;
						result.collision_point = b;
						result.collision_normal = (b - a).normalized();
						if (col_obj->get_type() == CollisionObject3DSW::TYPE_BODY) {
							Body3DSW *body = (Body3DSW *)col_obj;
							Vector3 rel_vec = b - body->get_transform().get_origin();
							result.collider_velocity = body->get_linear_velocity() + (body->get_angular_velocity()).cross(body->get_transform().origin - rel_vec);
						}
						result.collision_local_shape = body_shape_idx;
						result.collider_id = col_obj->get_instance_id();
						result.collider = col_obj->get_self();
						result.collider_shape = col_shape_idx;
						//result.collider_metadata = col_obj->get_shape_metadata(col_shape_idx);
					}
				}
			}
		}

		if (recover_motion == Vector3()) {
			break;
		}

		body_transform.origin += recover_motion;
		body_aabb.position += recover_motion;
	}

	r_recover_motion = body_transform.origin - p_xform.origin;
	return rays_found;
}

bool Space3DSW::test_body_motion(Body3DSW *p_body, const Transform &p_xform, const Vector3 &p_motion, bool p_infinite_inertia, real_t p_margin, PhysicsServer3D::MotionResult *r_result, bool p_exclude_ray_shapes) {
	if (r_result) {
		r_result->motion = p_motion;
		r_result->remainder = Vector3();
	}

	AABB body_aabb;
	bool shapes_found = false;

	for (int body_shape_idx = 0; body_shape_idx < p_body->get_shape_count(); body_shape_idx++) {
		if (p_body->is_shape_set_as_disabled(body_shape_idx)) {
			continue;
		}

		// Ignore ray shapes
		if (p_exclude_ray_shapes && p_body->get_shape(body_shape_idx)->get_type() == PhysicsServer3D::SHAPE_RAY) {
			continue;
		}

		if (!shapes_found) {
			body_aabb = p_body->get_shape_aabb(body_shape_idx);
			shapes_found = true;
		} else {
			body_aabb = body_aabb.merge(p_body->get_shape_aabb(body_shape_idx));
		}
	}

	if (!shapes_found) {
		return false;
	}

	// Undo the currently transform the physics server is aware of and apply the provided one
	body_aabb = p_xform.xform(p_body->get_inv_transform().xform(body_aabb));
	body_aabb = body_aabb.grow(p_margin);

	// Step 1: Try to free the body, if it is stuck

	Transform body_transform = p_xform;

	const int max_results = 32;
	Vector3 col_points[max_results * 2];

	CollisionSolver3DSW::CallbackResult col_cbk = PhysicsServer3DSW::_shape_col_cbk;
	PhysicsServer3DSW::CollCbkData col_cbk_data;
	col_cbk_data.max = max_results;
	col_cbk_data.ptr = col_points;

	// Don't separate by more than the intended motion
	real_t separation_margin = MIN(p_margin, MAX(0.0, p_motion.length() - CMP_EPSILON));

	for (int recover_attempt = 0; recover_attempt < 4; recover_attempt++) {
		// Reset counter
		col_cbk_data.amount = 0;

		int pair_count = _cull_aabb_for_body(p_body, body_aabb);

		for (int body_shape_idx = 0; body_shape_idx < p_body->get_shape_count(); body_shape_idx++) {
			if (p_body->is_shape_set_as_disabled(body_shape_idx)) {
				continue;
			}

			const Shape3DSW *body_shape = p_body->get_shape(body_shape_idx);

			// Ignore ray shapes
			if (p_exclude_ray_shapes && body_shape->get_type() == PhysicsServer3D::SHAPE_RAY) {
				continue;
			}

			const Transform body_shape_xform = body_transform * p_body->get_shape_transform(body_shape_idx);

			for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
				const CollisionObject3DSW *col_obj = intersection_query_results[pair_idx];
				int col_shape_idx = intersection_query_subindex_results[pair_idx];
				const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
				const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

				CollisionSolver3DSW::solve_static(body_shape, body_shape_xform, col_shape, col_shape_xform, col_cbk, &col_cbk_data, nullptr, separation_margin);
			}
		}

		Vector3 recover_motion;

		for (int i = 0; i < col_cbk_data.amount; i++) {
			Vector3 a = col_points[i * 2 + 0];
			Vector3 b = col_points[i * 2 + 1];
			recover_motion += (b - a) / col_cbk_data.amount;
		}

		if (recover_motion == Vector3()) {
			break;
		}

		body_transform.origin += recover_motion;
		body_aabb.position += recover_motion;
	}

	if (r_result) {
		r_result->motion += (body_transform.get_origin() - p_xform.get_origin());
	}

	// Step 2: Determine maximum possible motion without collision

	real_t safe = 1.0;
	real_t unsafe = 1.0;
	int best_shape = -1;

	AABB motion_aabb = body_aabb;
	motion_aabb.position += p_motion;
	motion_aabb = motion_aabb.merge(body_aabb);
	Vector3 motion_normal = p_motion.normalized();
	Vector3 point_A, point_B;

	int pair_count = _cull_aabb_for_body(p_body, motion_aabb);

	for (int body_shape_idx = 0; body_shape_idx < p_body->get_shape_count(); body_shape_idx++) {
		if (p_body->is_shape_set_as_disabled(body_shape_idx)) {
			continue;
		}

		Shape3DSW *body_shape = p_body->get_shape(body_shape_idx);

		// Ignore ray shapes
		if (p_exclude_ray_shapes && body_shape->get_type() == PhysicsServer3D::SHAPE_RAY) {
			continue;
		}

		const Transform body_shape_xform = body_transform * p_body->get_shape_transform(body_shape_idx);
		const Transform body_shape_xform_inv = body_shape_xform.affine_inverse();
		MotionShape3DSW mshape;
		mshape.shape = body_shape;
		mshape.motion = body_shape_xform_inv.basis.xform(p_motion);

		bool stuck = false;

		real_t best_safe = 1.0;
		real_t best_unsafe = 1.0;

		for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
			const CollisionObject3DSW *col_obj = intersection_query_results[pair_idx];
			int col_shape_idx = intersection_query_subindex_results[pair_idx];
			const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
			const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

			// Does it collide if going all the way?
			Vector3 sep_axis = motion_normal;
			if (CollisionSolver3DSW::solve_distance(&mshape, body_shape_xform, col_shape, col_shape_xform, point_A, point_B, motion_aabb, &sep_axis)) {
				continue;
			}

			// Is it stuck?
			sep_axis = motion_normal;
			if (!CollisionSolver3DSW::solve_distance(body_shape, body_shape_xform, col_shape, col_shape_xform, point_A, point_B, motion_aabb, &sep_axis)) {
				stuck = true;
				break;
			}

			// Do kinematic solving
			real_t low = 0;
			real_t hi = 1;

			for (int step = 0; step < 8; step++) { // Steps should be customizable.
				real_t ofs = (low + hi) * 0.5;

				sep_axis = motion_normal; // Important optimization for this to work fast enough.
				mshape.motion = body_shape_xform_inv.basis.xform(p_motion * ofs);
				if (CollisionSolver3DSW::solve_distance(&mshape, body_shape_xform, col_shape, col_shape_xform, point_A, point_B, motion_aabb, &sep_axis)) {
					low = ofs;
				} else {
					hi = ofs;
				}
			}

			if (low < best_safe) {
				best_safe = low;
				best_unsafe = hi;
			}
		}

		if (stuck) {
			safe = 0;
			unsafe = 0;
			best_shape = body_shape_idx; // Sadly, it's the best
			break;
		}

		if (best_safe < safe) {
			safe = best_safe;
			unsafe = best_unsafe;
			best_shape = body_shape_idx;
		}
	}

	// Step 3: If it collided, retrieve collision information using the unsafe distance.

	if (safe < 1 && r_result) {
		Transform ugt = body_transform;
		ugt.origin += p_motion * unsafe;

		_RestCallbackData rcd;
		rcd.min_allowed_depth = test_motion_min_contact_depth;

		const Transform body_shape_xform = ugt * p_body->get_shape_transform(best_shape);
		const Shape3DSW *body_shape = p_body->get_shape(best_shape);

		body_aabb.position += p_motion * unsafe;

		pair_count = _cull_aabb_for_body(p_body, body_aabb);

		for (int pair_idx = 0; pair_idx < pair_count; pair_idx++) {
			const CollisionObject3DSW *col_obj = intersection_query_results[pair_idx];
			int col_shape_idx = intersection_query_subindex_results[pair_idx];
			const Shape3DSW *col_shape = col_obj->get_shape(col_shape_idx);
			const Transform col_shape_xform = col_obj->get_transform() * col_obj->get_shape_transform(col_shape_idx);

			rcd.object = col_obj;
			rcd.shape = col_shape_idx;
			CollisionSolver3DSW::solve_static(body_shape, body_shape_xform, col_shape, col_shape_xform, _rest_cbk_result, &rcd, nullptr, p_margin);
		}

		ERR_FAIL_COND_V_MSG(rcd.best_len == 0, false, "Failed to extract collision information");

		r_result->motion = safe * p_motion;
		r_result->remainder = p_motion - safe * p_motion;
		r_result->motion += (body_transform.get_origin() - p_xform.get_origin());

		r_result->collider = rcd.best_object->get_self();
		r_result->collider_id = rcd.best_object->get_instance_id();
		r_result->collider_shape = rcd.best_shape;
		r_result->collision_local_shape = best_shape;
		r_result->collision_normal = rcd.best_normal;
		r_result->collision_point = rcd.best_contact;
		const Body3DSW *body = static_cast<const Body3DSW *>(rcd.best_object);
		r_result->collider_velocity = body->get_linear_velocity() + (body->get_angular_velocity()).cross(body->get_transform().origin - rcd.best_contact);
	}

	return (safe < 1);
}

void *Space3DSW::_broadphase_pair(CollisionObject3DSW *A, int p_subindex_A, CollisionObject3DSW *B, int p_subindex_B, void *p_self) {
	if (!A->test_collision_mask(B)) {
		return nullptr;
	}

	CollisionObject3DSW::Type type_A = A->get_type();
	CollisionObject3DSW::Type type_B = B->get_type();
	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(p_subindex_A, p_subindex_B);
		SWAP(type_A, type_B);
	}

	Space3DSW *self = (Space3DSW *)p_self;
	self->collision_pairs++;

	if (type_A == CollisionObject3DSW::TYPE_AREA) {
		Area3DSW *area = static_cast<Area3DSW *>(A);
		if (type_B == CollisionObject3DSW::TYPE_AREA) {
			Area3DSW *area_b = static_cast<Area3DSW *>(B);
			Area2Pair3DSW *area2_pair = memnew(Area2Pair3DSW(area_b, p_subindex_B, area, p_subindex_A));
			return area2_pair;
		} else {
			Body3DSW *body = static_cast<Body3DSW *>(B);
			AreaPair3DSW *area_pair = memnew(AreaPair3DSW(body, p_subindex_B, area, p_subindex_A));
			return area_pair;
		}
	} else {
		BodyPair3DSW *b = memnew(BodyPair3DSW((Body3DSW *)A, p_subindex_A, (Body3DSW *)B, p_subindex_B));
		return b;
	}

	return nullptr;
}

void Space3DSW::_broadphase_unpair(CollisionObject3DSW *A, int p_subindex_A, CollisionObject3DSW *B, int p_subindex_B, void *p_data, void *p_self) {
	if (!p_data) {
		return;
	}

	Space3DSW *self = (Space3DSW *)p_self;
	self->collision_pairs--;
	Constraint3DSW *c = (Constraint3DSW *)p_data;
	memdelete(c);
}

const SelfList<Body3DSW>::List &Space3DSW::get_active_body_list() const {
	return active_list;
}

void Space3DSW::body_add_to_active_list(SelfList<Body3DSW> *p_body) {
	active_list.add(p_body);
}

void Space3DSW::body_remove_from_active_list(SelfList<Body3DSW> *p_body) {
	active_list.remove(p_body);
}

void Space3DSW::body_add_to_inertia_update_list(SelfList<Body3DSW> *p_body) {
	inertia_update_list.add(p_body);
}

void Space3DSW::body_remove_from_inertia_update_list(SelfList<Body3DSW> *p_body) {
	inertia_update_list.remove(p_body);
}

BroadPhase3DSW *Space3DSW::get_broadphase() {
	return broadphase;
}

void Space3DSW::add_object(CollisionObject3DSW *p_object) {
	ERR_FAIL_COND_MSG(objects.has(p_object), "Object already in space");
	objects.insert(p_object);
}

void Space3DSW::remove_object(CollisionObject3DSW *p_object) {
	ERR_FAIL_COND_MSG(!objects.has(p_object), "Object not in space");
	objects.erase(p_object);
}

const Set<CollisionObject3DSW *> &Space3DSW::get_objects() const {
	return objects;
}

void Space3DSW::body_add_to_state_query_list(SelfList<Body3DSW> *p_body) {
	state_query_list.add(p_body);
}

void Space3DSW::body_remove_from_state_query_list(SelfList<Body3DSW> *p_body) {
	state_query_list.remove(p_body);
}

void Space3DSW::area_add_to_monitor_query_list(SelfList<Area3DSW> *p_area) {
	monitor_query_list.add(p_area);
}

void Space3DSW::area_remove_from_monitor_query_list(SelfList<Area3DSW> *p_area) {
	monitor_query_list.remove(p_area);
}

void Space3DSW::area_add_to_moved_list(SelfList<Area3DSW> *p_area) {
	area_moved_list.add(p_area);
}

void Space3DSW::area_remove_from_moved_list(SelfList<Area3DSW> *p_area) {
	area_moved_list.remove(p_area);
}

const SelfList<Area3DSW>::List &Space3DSW::get_moved_area_list() const {
	return area_moved_list;
}

void Space3DSW::call_queries() {
	while (state_query_list.first()) {
		Body3DSW *b = state_query_list.first()->self();
		state_query_list.remove(state_query_list.first());
		b->call_queries();
	}

	while (monitor_query_list.first()) {
		Area3DSW *a = monitor_query_list.first()->self();
		monitor_query_list.remove(monitor_query_list.first());
		a->call_queries();
	}
}

void Space3DSW::setup() {
	contact_debug_count = 0;
	while (inertia_update_list.first()) {
		inertia_update_list.first()->self()->update_inertias();
		inertia_update_list.remove(inertia_update_list.first());
	}
}

void Space3DSW::update() {
	broadphase->update();
}

void Space3DSW::set_param(PhysicsServer3D::SpaceParameter p_param, real_t p_value) {
	switch (p_param) {
		case PhysicsServer3D::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
			contact_recycle_radius = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_CONTACT_MAX_SEPARATION:
			contact_max_separation = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_BODY_MAX_ALLOWED_PENETRATION:
			contact_max_allowed_penetration = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
			body_linear_velocity_sleep_threshold = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
			body_angular_velocity_sleep_threshold = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_BODY_TIME_TO_SLEEP:
			body_time_to_sleep = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_BODY_ANGULAR_VELOCITY_DAMP_RATIO:
			body_angular_velocity_damp_ratio = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
			constraint_bias = p_value;
			break;
		case PhysicsServer3D::SPACE_PARAM_TEST_MOTION_MIN_CONTACT_DEPTH:
			test_motion_min_contact_depth = p_value;
			break;
	}
}

real_t Space3DSW::get_param(PhysicsServer3D::SpaceParameter p_param) const {
	switch (p_param) {
		case PhysicsServer3D::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
			return contact_recycle_radius;
		case PhysicsServer3D::SPACE_PARAM_CONTACT_MAX_SEPARATION:
			return contact_max_separation;
		case PhysicsServer3D::SPACE_PARAM_BODY_MAX_ALLOWED_PENETRATION:
			return contact_max_allowed_penetration;
		case PhysicsServer3D::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
			return body_linear_velocity_sleep_threshold;
		case PhysicsServer3D::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
			return body_angular_velocity_sleep_threshold;
		case PhysicsServer3D::SPACE_PARAM_BODY_TIME_TO_SLEEP:
			return body_time_to_sleep;
		case PhysicsServer3D::SPACE_PARAM_BODY_ANGULAR_VELOCITY_DAMP_RATIO:
			return body_angular_velocity_damp_ratio;
		case PhysicsServer3D::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
			return constraint_bias;
		case PhysicsServer3D::SPACE_PARAM_TEST_MOTION_MIN_CONTACT_DEPTH:
			return test_motion_min_contact_depth;
	}

	ERR_FAIL_COND_V_MSG(true, 0, "Unknown space parameter");
}

void Space3DSW::lock() {
	locked = true;
}

void Space3DSW::unlock() {
	locked = false;
}

bool Space3DSW::is_locked() const {
	return locked;
}

PhysicsDirectSpaceState3DSW *Space3DSW::get_direct_state() {
	return direct_access;
}

Space3DSW::Space3DSW() {
	ProjectSettings::get_singleton()->set_custom_property_info("physics/3d/time_before_sleep", PropertyInfo(Variant::FLOAT, "physics/3d/time_before_sleep", PROPERTY_HINT_RANGE, "0,5,0.01,or_greater"));

	broadphase = BroadPhase3DSW::create_func();
	broadphase->set_pair_callback(_broadphase_pair, this);
	broadphase->set_unpair_callback(_broadphase_unpair, this);

	direct_access = memnew(PhysicsDirectSpaceState3DSW);
	direct_access->space = this;

	for (int i = 0; i < ELAPSED_TIME_MAX; i++) {
		elapsed_time[i] = 0;
	}
}

Space3DSW::~Space3DSW() {
	memdelete(broadphase);
	memdelete(direct_access);
}
