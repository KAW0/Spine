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
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2019 Clockwork Origins

#pragma once

#include <QString>

namespace spine {
namespace utils {

	class Compression {
	public:
		static bool compress(const QString & file, bool deleteSourceAfterCompression);
		static bool compress(const QString & file, const QString & targetFile, bool deleteSourceAfterCompression);
		static bool uncompress(const QString & file, bool deleteSourceAfterUncompression);
		static bool uncompress(const QString & file, const QString & targetFile, bool deleteSourceAfterUncompression);
	};

} /* namespace utils */
} /* namespace spine */