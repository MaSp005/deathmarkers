#include "cluster.hpp"

using namespace dm;

DeathLocationStack::DeathLocationStack(vector<DeathLocation*> deaths) {
	this->deaths = deaths;
	this->recalculate();
}

DeathLocationStack::DeathLocationStack() {
	this->deaths = vector<DeathLocation*>();
	this->recalculate();
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
CCPoint averagePos(vector<DeathLocation*>::iterator const begin,
	vector<DeathLocation*>::iterator const end) {
	double avgX = 0;
	double avgY = 0;

	for (auto death = begin; death < end; death++) {
		avgX += (*death)->pos.x;
		avgY += (*death)->pos.y;
	}

	avgX /= end - begin;
	avgY /= end - begin;

	return CCPoint(avgX, avgY);
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
void DeathLocationStack::recalculate() {
	vector<CCPoint> points;
	for (auto i = this->deaths.begin(); i < this->deaths.end(); i++)
		points.push_back((*i)->pos);
	this->circle = makeSmallestEnclosingCircle(points);
	this->density = this->circle.r ? static_cast<float>(this->deaths.size()) / (this->circle.r * this->circle.r) : -1;
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
DeathLocationStack mergeStacks(vector<DeathLocationStack>* stacks,
	vector<DeathLocationStack>::iterator a,
	vector<DeathLocationStack>::iterator b) {
	// Merge into new Stack
	auto merged = DeathLocationStack();
	merged.deaths.reserve(a->deaths.size() + b->deaths.size());
	merged.deaths.insert(merged.deaths.end(), a->deaths.begin(), a->deaths.end());
	merged.deaths.insert(merged.deaths.end(), b->deaths.begin(), b->deaths.end());
	merged.recalculate();
	return merged;
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
int findNearest(vector<DeathLocationStack> const* stacks,
	vector<DeathLocationStack>::iterator source, float maxDistance) {
	// Assumes death stacks vector is (roughly) sorted by x-coordinate

	CCPoint srcPoint = source->circle.c;
	float minDistSq = maxDistance;
	int minimum = -1;
	
	// Walk right
	for (auto i = source + 1; i < stacks->end(); i++) {
		// All points from here will be out of range on x alone, skip
		if (i->circle.c.x > srcPoint.x + maxDistance) break;
		float dist = i->circle.c.getDistance(srcPoint);
		if (dist < minDistSq) {
			minimum = i - stacks->begin();
			minDistSq = dist;
		}
	}
	// Walk left
	for (auto i = source - 1; i >= stacks->begin(); i--) {
		if (i->circle.c.x < srcPoint.x - maxDistance) break;
		float dist = i->circle.c.getDistance(srcPoint);
		if (dist < minDistSq) {
			minimum = i - stacks->begin();
			minDistSq = dist;
		}
		if (i == stacks->begin()) break;
	}

	return minimum;
}

void dm::identifyClusters(vector<DeathLocation>* deaths,
	float maxDistance, vector<DeathLocationStack>* stacks) {

	/*
	*  Hierarchical Clustering
	*  -----------------------
	*  Turn each node into a single-element cluster,
	*  repeatedly merge all clusters until no more merging can take place
	*  Assumes deaths vector is sorted by x-coordinate
	*/

	log::debug("Clustering {} entries with maximum distance {}",
		deaths->size(), maxDistance);
	stacks->clear();
	stacks->reserve(deaths->size());

	for (auto i = deaths->begin(); i < deaths->end(); i++) {
		vector<DeathLocation*> vector;
		i->clustered = false;
		vector.push_back(&*i);
		stacks->push_back(DeathLocationStack(vector));
	}
	// At this point, death stacks vector is also sorted by x-coordinate
	
	int iter = 0;
	while (true) {
		bool didMerge = false;
		iter++;

		for (auto i = stacks->begin(); i < stacks->end();) {
			auto maxMergeDist = maxDistance - i->circle.r * 2;
			if (maxMergeDist < 0) {
				i++;
				continue;
			}
			int nearestIdx = findNearest(stacks, i, maxMergeDist);
			if (nearestIdx == -1) {
				if (i->deaths.size() <= 1) {
					// This can only happen in the first iteration,
					// otherwise it will have been merged and have > 1 elements
					stacks->erase(i);
					continue;
				}
				i++;
				continue;
			}
			auto nearest = stacks->begin() + nearestIdx;
			auto merged = mergeStacks(stacks, i, nearest);

			if (
				i->circle.r != 0 && nearest->circle.r != 0 &&
				log2(maxMergeDist) * merged.density < min(i->density, nearest->density) / 4
			) {
				i++;
				continue;
			}

			// Replace entries in stack vector
			if (i < nearest) {              // ..., i, ..., nearest, ...
			    stacks->erase(i);           // ..., (i), ..., nearest, (nearest), ...
			    stacks->erase(--nearest);   // ..., (i), ..., (nearest), ...
			} else {                        // ..., nearest, ..., i, ...
			    stacks->erase(nearest);     // ..., (nearest), ..., i, (i), ...
			    stacks->erase(--i);         // ..., (nearest), ..., (i), ...
			}
			stacks->insert(i, merged);
			i++;

			didMerge = true;
		}

		if (!didMerge) break;
	}

	erase_if(*stacks, [maxDistance](const DeathLocationStack& stack) {
		for (auto i = stack.deaths.begin(); i < stack.deaths.end(); i++) {
			(*i)->clustered = true;
		}
		if (stack.deaths.size() == 1) return true;
		// if (stack.diameter == 0) return false;
		return false;
	});

	log::debug("Finished clustering into {} stacks.", stacks->size());
}