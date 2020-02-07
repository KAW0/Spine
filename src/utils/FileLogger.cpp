/*
	This file is part of Spine.

    Spine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spine.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2020 Clockwork Origins

#include "FileLogger.h"

namespace spine {
namespace utils {

	FileLogger::FileLogger(const std::string & path) : _fileStream(path) {
	}

	clockUtils::log::SinkWrapper & FileLogger::operator<<(const std::string & str) {
		_fileStream << str;
		_fileStream.flush();
		return *this;
	}
	
	bool FileLogger::isSame(void * sink) const {
		return sink == this;
	}
	
} /* namespace utils */
} /* namespace spine */