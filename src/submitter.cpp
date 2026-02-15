#include "submitter.hpp"


Submitter::Submitter(web::WebRequest request) {
	request.timeout(dm::HTTP_TIMEOUT);
  this->request = request;
}

Submitter::~Submitter() {}

void Submitter::event(WebResponse res) {
	if (res.ok()) {
		log::debug("Posted Deaths.");
		drop();
		return;
	}

	int code = res.code();
	auto body = res.string().unwrapOr("Unknown");

	log::error("Posting Deaths failed: {} {}", code, body);

	if (code == 429) {
		auto timeoutHeader = res.header("Retry-After");
		auto timeout = timeoutHeader.has_value() ?
			std::chrono::seconds(atoi(timeoutHeader->c_str())) :
			RETRY_TIMEOUT;

		log::debug("Hit rate limit, using Retry-After header...");
		std::thread([this, timeout]() {
			std::this_thread::sleep_for(timeout);
			this->submit();
		}).detach();

	} else if (code >= 400 && code < 500) {
		log::warn("Dropping submission due to 4xx error response.");
		drop();

	} else if (code >= 500 && code < 600 || code < 0) { // whatever 6xx responses mean
		log::debug("Waiting to retry...");
		std::thread([this]() {
			std::this_thread::sleep_for(RETRY_TIMEOUT);
			this->submit();
		}).detach();
	}
}

void Submitter::drop() {
	delete this;
}

void Submitter::submit() {
	log::debug("Sending request... (Retry {})", this->retries);
	if (retries >= MAX_RETRIES) {
		log::warn("Hit maximum retry count. Dropping submission.");
		drop();
		return;
	}
	retries++;
	this->listener.spawn(
		this->request.get(dm::makeRequestURL("submit")),
		[this](WebResponse e) {
			this->event(e);
		}
	);
}


void dm::purgeSpam(vector<DeathLocationOut>& deaths) {
	int total = deaths.size();

	if (total < 10) return;
	log::debug("Spam Removal: 10 death threshold passed.");

	std::time_t timeDiff = deaths.back().realTime - deaths.front().realTime;
	log::debug("Spam Removal: {} deaths over {} seconds.", total, timeDiff);
	// Limit to 1 death per 1.5 seconds
	if (timeDiff * 2 < static_cast<long long>(total) * 3) {
		deaths.clear();
		return;
	}
	log::debug("Spam Removal: Time check passed.");

	if (total < 20) return;
	log::debug("Spam Removal: 20 death threshold passed.");

	unordered_map<int, uint32_t> xFrequency;
	for (auto i = deaths.begin(); i < deaths.end(); i++) {
		int translatedX = translate ( i->pos.x );
		if (xFrequency.contains(translatedX)) xFrequency[translatedX] = 0;
		xFrequency[translatedX]++;
	}

	uint32_t max = 0;
	for (auto& [x, count] : xFrequency) {
		if (count > max) max = count;
	}
	log::debug("Spam Removal: Maximum concentration {}", max);

	if (max * max / 2 < total) return;
	log::debug("Spam Removal: Square Threshold passed.");

	log::debug("Spam Removal: Pre-purge {}", total);
	erase_if(deaths, [&xFrequency, max](auto& d){
		int translatedX = translate ( d.pos.x );
		return static_cast<float>(xFrequency[translatedX]) / max > .8;
	});
	log::debug("Spam Removal: Post-purge {}", deaths.size());
}
