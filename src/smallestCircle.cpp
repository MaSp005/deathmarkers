/* 
 * Smallest enclosing circle - Library (C++)
 * 
 * Copyright (c) 2021 Project Nayuki
 * https://www.nayuki.io/page/smallest-enclosing-circle
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program (see COPYING.txt and COPYING.LESSER.txt).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include "smallestCircle.hpp"

using std::size_t;
using std::vector;
using std::max;
using std::min;
using namespace dm;


/*---- Members of struct Circle ----*/

const Circle Circle::INVALID{CCPoint{0, 0}, -1};

const double Circle::MULTIPLICATIVE_EPSILON = 1 + 1e-14;


bool Circle::contains(const CCPoint &p) const {
	return c.getDistance(p) <= r * MULTIPLICATIVE_EPSILON;
}


bool Circle::contains(const vector<CCPoint> &ps) const {
	for (const CCPoint &p : ps) {
		if (!contains(p))
			return false;
	}
	return true;
}


/*---- Smallest enclosing circle algorithm ----*/

static Circle makeSmallestEnclosingCircleOnePoint (const vector<CCPoint> &points, size_t end, const CCPoint &p);
static Circle makeSmallestEnclosingCircleTwoPoints(const vector<CCPoint> &points, size_t end, const CCPoint &p, const CCPoint &q);

static std::default_random_engine randGen((std::random_device())());


// Initially: No boundary points known
Circle dm::makeSmallestEnclosingCircle(vector<CCPoint> points) {
	// Randomize order
	std::shuffle(points.begin(), points.end(), randGen);
	
	// Progressively add points to circle or recompute circle
	Circle c = Circle::INVALID;
	for (size_t i = 0; i < points.size(); i++) {
		const CCPoint &p = points.at(i);
		if (c.r < 0 || !c.contains(p))
			c = makeSmallestEnclosingCircleOnePoint(points, i + 1, p);
	}
	return c;
}


// One boundary point known
static Circle makeSmallestEnclosingCircleOnePoint(const vector<CCPoint> &points, size_t end, const CCPoint &p) {
	Circle c{p, 0};
	for (size_t i = 0; i < end; i++) {
		const CCPoint &q = points.at(i);
		if (!c.contains(q)) {
			if (c.r == 0)
				c = dm::makeDiameter(p, q);
			else
				c = makeSmallestEnclosingCircleTwoPoints(points, i + 1, p, q);
		}
	}
	return c;
}


// Two boundary points known
static Circle makeSmallestEnclosingCircleTwoPoints(const vector<CCPoint> &points, size_t end, const CCPoint &p, const CCPoint &q) {
	Circle circ = dm::makeDiameter(p, q);
	Circle left  = Circle::INVALID;
	Circle right = Circle::INVALID;
	
	// For each point not in the two-point circle
	CCPoint pq = q - p;
	for (size_t i = 0; i < end; i++) {
		const CCPoint &r = points.at(i);
		if (circ.contains(r))
			continue;
		
		// Form a circumcircle and classify it on left or right side
		double cross = pq.cross(r - p);
		Circle c = dm::makeCircumcircle(p, q, r);
		if (c.r < 0)
			continue;
		else if (cross > 0 && (left.r < 0 || pq.cross(c.c - p) > pq.cross(left.c - p)))
			left = c;
		else if (cross < 0 && (right.r < 0 || pq.cross(c.c - p) < pq.cross(right.c - p)))
			right = c;
	}
	
	// Select which circle to return
	if (left.r < 0 && right.r < 0)
		return circ;
	else if (left.r < 0)
		return right;
	else if (right.r < 0)
		return left;
	else
		return left.r <= right.r ? left : right;
}


Circle dm::makeDiameter(const CCPoint &a, const CCPoint &b) {
	CCPoint c{(a.x + b.x) / 2, (a.y + b.y) / 2};
	return Circle{c, max(c.getDistance(a), c.getDistance(b))};
}


Circle dm::makeCircumcircle(const CCPoint &a, const CCPoint &b, const CCPoint &c) {
	// Mathematical algorithm from Wikipedia: Circumscribed circle
	float ox = (min(min(a.x, b.x), c.x) + max(max(a.x, b.x), c.x)) / 2;
	float oy = (min(min(a.y, b.y), c.y) + max(max(a.y, b.y), c.y)) / 2;
	float ax = a.x - ox,  ay = a.y - oy;
	float bx = b.x - ox,  by = b.y - oy;
	float cx = c.x - ox,  cy = c.y - oy;
	float d = (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by)) * 2;
	if (d == 0)
		return Circle::INVALID;
	float x = ((ax*ax + ay*ay) * (by - cy) + (bx*bx + by*by) * (cy - ay) + (cx*cx + cy*cy) * (ay - by)) / d;
	float y = ((ax*ax + ay*ay) * (cx - bx) + (bx*bx + by*by) * (ax - cx) + (cx*cx + cy*cy) * (bx - ax)) / d;
	CCPoint p{ox + x, oy + y};
	float r = max(max(p.getDistance(a), p.getDistance(b)), p.getDistance(c));
	return Circle{p, r};
}
