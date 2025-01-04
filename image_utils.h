#pragma once
#include <aegis.hpp>
#include <filesystem>
#include <Magick++.h>
#include "string_utils.h"
#include "aegis/gateway/objects/messages.hpp"

const std::filesystem::path cache_path("image_cache\\");

class ImageManipulator {
public:

	ImageManipulator() = default;

	ImageManipulator(std::filesystem::path url)  {
		this->initialize(url);
	}

	ImageManipulator& initialize(std::filesystem::path url) {
		this->path = fmt::format("{}{}", cache_path.string(), url.filename().string());
		this->url = url.string();

		if (!ImageManipulator::supported_type(url)) return *this;

		std::string extension = this->path.extension().string();
		to_lower(extension);

		if (extension == ".gif" || extension == ".webp") { // Multi-image formats
			Magick::readImages(&this->images, this->url);
		}
		else if (extension == ".png" || extension == ".jpeg" || extension == ".jpg") { // Single-image formats
			this->images = { Magick::Image(this->url) };
		}

		this->_initialized = true;
		return *this;
	}

	ImageManipulator& manipulate(std::function<void(Magick::Image*)>&& Func, std::vector<Magick::Image>::iterator begin, std::vector<Magick::Image>::iterator end) {
		for (auto iter = begin; iter != end; ++iter) {
			Func(iter._Ptr);
		}
		return *this;
	}

	aegis::rest::aegis_file get_file() {

		if (!this->_initialized) throw std::runtime_error("Not initialized.");

		this->wait_for_unblock();
		size_t image_count = this->images.size();
		if (image_count > 1)
			Magick::writeImages(this->images.begin(), this->images.end(), path.string());
		else if (image_count == 1)
			this->images[0].write(path.string());
		else return { "null", {0} };

		this->wait_for_unblock();

		std::ifstream reader(this->path, std::ifstream::binary);
		reader.seekg(0, reader.end);
		int length = reader.tellg();
		reader.seekg(0, reader.beg);

		char* buffer = new char[length];

		reader.read(buffer, length);
		reader.close();

		std::vector<char> vector_data;
		std::copy(buffer, buffer + length, std::back_inserter(vector_data));

		delete[] buffer;
		std::filesystem::remove(this->path);

		return { fmt::format("output{}", this->path.extension().string()), vector_data };
	}

	inline void wait_for_unblock() const noexcept {
		while (std::filesystem::is_block_file(this->path)) {
			std::this_thread::sleep_for(100ms);
		}
	}

	inline Magick::Image* base_image() const noexcept {
		return this->images.begin()._Ptr;
	}

	inline bool initialized() const noexcept { return this->_initialized; };

	double point_size(std::string text, size_t width = 0, size_t height = 0) {
		if (width <= 0) {
			width = this->base_image()->size().width();
		}
		if (height <= 0) {
			height = this->base_image()->size().height();
		}

		double h_w_scale = std::clamp(2.0 * static_cast<double>(height / width), 0.0, 2.0);
		double w_h_scale = std::clamp(2.0 * static_cast<double>(width / height), 0.0, 2.0);

		Magick::TypeMetric tm;
		double pointsize = 128.00 * (std::sqrt(std::pow(width, h_w_scale) + std::pow(height, w_h_scale)) / 600);

		this->base_image()->fontPointsize(pointsize);
		this->base_image()->fontTypeMetricsMultiline(text, &tm);
		while (tm.textWidth() > this->base_image()->size().width() || tm.textHeight() > this->base_image()->size().height()) {
			if (pointsize < 2.0) break;
			pointsize *= 0.8;
			this->base_image()->fontPointsize(pointsize);
			this->base_image()->fontTypeMetricsMultiline(text, &tm);
		}

		return pointsize;
	}

	double stroke_width(double point_size) {
		return std::clamp(point_size / 26, 1.0, 1000.0);
	}

	inline static bool supported_type(std::filesystem::path url) {
		std::string extension = url.extension().string();
		to_lower(extension);
		for (const std::string& type : supported_types) {
			if (type == extension) return true;
		}
		return false;
	}

	static bool get_from_message(aegis::gateway::objects::message msg, ImageManipulator& im, size_t max_size = 0) {

		if (!msg.attachments.empty()) {
			if (max_size > 0 && msg.attachments[0].size > max_size) return false;
			im.initialize(msg.attachments[0].proxy_url);
		}
		else if (max_size == 0) { // If no attachments and no max size, find embed or link
			std::smatch matches;
			std::regex_match(msg.get_content(), matches, std::basic_regex("http\\S+", std::regex_constants::icase)); // Find links in message

			if (!matches.empty()) {
				im.initialize(matches.begin()->str());
			}
			else if (!msg.embeds.empty() && !msg.embeds[0].image().proxy_url.empty()) {
				im.initialize(msg.embeds[0].image().proxy_url);
			}
			else if (!msg.embeds.empty() && !msg.embeds[0].thumbnail().proxy_url.empty()) {
				im.initialize(msg.embeds[0].thumbnail().proxy_url);
			}
		}
		if (!im.initialized()) { // If couldn't find anything in current message, look through previous messages
			aegis::gateway::objects::messages msgs = msg.get_channel().get_messages(aegis::get_messages_t().before().message_id(msg.get_id()).limit(50)).get(); // Get last 50 msgs
			for (aegis::gateway::objects::message& msg : msgs._messages) {
				if (!msg.attachments.empty()) {
					if (max_size > 0 && msg.attachments[0].size > max_size) return false;
					im.initialize(msg.attachments[0].proxy_url);
					break;
				}
				else if (max_size == 0) { // If no attachments and no max size, find embed or link
					std::smatch matches;
					std::regex_match(msg.get_content(), matches, std::basic_regex("http\S+", std::regex_constants::icase));
					if (!matches.empty()) {
						im.initialize(matches.begin()->str());
					}
					else if (msg.embeds.begin() != msg.embeds.end() && !msg.embeds[0].image().proxy_url.empty()) {
						im.initialize(msg.embeds[0].image().proxy_url);
						break;
					}
					else if (msg.embeds.begin() != msg.embeds.end() && !msg.embeds[0].thumbnail().proxy_url.empty()) {
						im.initialize(msg.embeds[0].thumbnail().proxy_url);
						break;
					}
				}
			}
		}
		return im.initialized();
	}

	std::vector<Magick::Image> images;
	std::string url;
	std::filesystem::path path;

	static std::vector<std::string> supported_types;

private:
	bool _initialized = false;
};

std::vector<std::string> ImageManipulator::supported_types {
	".gif", ".webp", ".png", ".jpeg", ".jpg"
};