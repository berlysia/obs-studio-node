// Client module for the OBS Studio node module.
// Copyright(C) 2017 Streamlabs (General Workings Inc)
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.

#include "osn-global.hpp"
#include <obs.h>
#include "osn-source.hpp"
#include <error.hpp>

void osn::Global::Register(ipc::server& srv) {
	std::shared_ptr<ipc::collection> cls = std::make_shared<ipc::collection>("Global");
	cls->register_function(std::make_shared<ipc::function>("GetOutputSource", std::vector<ipc::type>{ipc::type::UInt32}, GetOutputSource));
	cls->register_function(std::make_shared<ipc::function>("SetOutputSource", std::vector<ipc::type>{ipc::type::UInt32, ipc::type::UInt64}, SetOutputSource));
	cls->register_function(std::make_shared<ipc::function>("LaggedFrames", std::vector<ipc::type>{ipc::type::String, ipc::type::String}, LaggedFrames));
	cls->register_function(std::make_shared<ipc::function>("TotalFrames", std::vector<ipc::type>{ipc::type::String, ipc::type::String}, TotalFrames));
	srv.register_collection(cls);
}

void osn::Global::GetOutputSource(void* data, const int64_t id, const std::vector<ipc::value>& args, std::vector<ipc::value>& rval) {
	obs_source_t* source = obs_get_output_source(args[0].value_union.ui32);
	if (!source) {
		rval.push_back(ipc::value((uint64_t)ErrorCode::Ok));
		rval.push_back(ipc::value(UINT64_MAX));
		rval.push_back(ipc::value(-1));
		return;
	}

	uint64_t uid = osn::Source::GetInstance()->Get(source);
	if (uid == UINT64_MAX) {
#ifdef DEBUG // Debug should throw an error for debuggers to catch.
		throw std::runtime_error("Source found but not indexed.");
#endif
		rval.push_back(ipc::value((uint64_t)ErrorCode::CriticalError));
		rval.push_back(ipc::value("Source found but not indexed."));
		return;
	}

	rval.push_back(ipc::value((uint64_t)ErrorCode::Ok));
	rval.push_back(ipc::value(uid));
	rval.push_back(ipc::value(obs_source_get_type(source)));
}

void osn::Global::SetOutputSource(void* data, const int64_t id, const std::vector<ipc::value>& args, std::vector<ipc::value>& rval) {
	obs_source_t* source = nullptr;
	
	if (args[1].value_union.ui64 != UINT64_MAX) {
		source = osn::Source::GetInstance()->Get(args[1].value_union.ui64);
		if (!source) {
			rval.push_back(ipc::value((uint64_t)ErrorCode::InvalidReference));
			rval.push_back(ipc::value("Source reference is not valid."));
			return;
		}
	}

	obs_set_output_source(args[0].value_union.ui32, source);
	if (obs_get_output_source(args[0].value_union.ui32) != source) {
		rval.push_back(ipc::value((uint64_t)ErrorCode::Error));
		rval.push_back(ipc::value("Failed to set output source."));
	} else {
		rval.push_back(ipc::value((uint64_t)ErrorCode::Ok));
	}
}

void osn::Global::LaggedFrames(void* data, const int64_t id, const std::vector<ipc::value>& args, std::vector<ipc::value>& rval) {
	rval = args; // !FIXME!
}

void osn::Global::TotalFrames(void* data, const int64_t id, const std::vector<ipc::value>& args, std::vector<ipc::value>& rval) {
	rval = args; // !FIXME!
}