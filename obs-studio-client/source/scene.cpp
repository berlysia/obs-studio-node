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

#include "scene.hpp"
#include "shared.hpp"
#include "utility.hpp"
#include "error.hpp"
#include "controller.hpp"
#include "ipc-value.hpp"
#include <string>
#include <condition_variable>
#include <mutex>
#include "input.hpp"
#include "sceneitem.hpp"

osn::Scene::Scene(uint64_t id) {
	this->sourceId = id;
}

Nan::Persistent<v8::FunctionTemplate> osn::Scene::prototype = Nan::Persistent<v8::FunctionTemplate>();

void osn::Scene::Register(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
	auto fnctemplate = Nan::New<v8::FunctionTemplate>();
	fnctemplate->Inherit(Nan::New<v8::FunctionTemplate>(osn::ISource::prototype));
	fnctemplate->InstanceTemplate()->SetInternalFieldCount(1);
	fnctemplate->SetClassName(Nan::New<v8::String>("Scene").ToLocalChecked());

	// Class Template
	utilv8::SetTemplateField(fnctemplate, "create", Create);
	utilv8::SetTemplateField(fnctemplate, "createPrivate", CreatePrivate);
	utilv8::SetTemplateField(fnctemplate, "fromName", FromName);

	// Prototype/Class Template
	v8::Local<v8::ObjectTemplate> objtemplate = fnctemplate->PrototypeTemplate();
	utilv8::SetTemplateField(objtemplate, "release", Release);
	utilv8::SetTemplateField(objtemplate, "remove", Remove);

	utilv8::SetTemplateAccessorProperty(objtemplate, "source", AsSource);
	utilv8::SetTemplateField(objtemplate, "duplicate", Duplicate);
	utilv8::SetTemplateField(objtemplate, "add", AddSource);
	utilv8::SetTemplateField(objtemplate, "findItem", FindItem);
	utilv8::SetTemplateField(objtemplate, "moveItem", MoveItem);
	utilv8::SetTemplateField(objtemplate, "getItemAtIdx", GetItemAtIndex);
	utilv8::SetTemplateField(objtemplate, "getItems", GetItems);
	utilv8::SetTemplateField(objtemplate, "getItemsInRange", GetItemsInRange);
	utilv8::SetTemplateField(objtemplate, "connect", Connect);
	utilv8::SetTemplateField(objtemplate, "disconnect", Disconnect);

	// Stuff
	utilv8::SetObjectField(target, "Scene", fnctemplate->GetFunction());
	prototype.Reset(fnctemplate);
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Create(Nan::NAN_METHOD_ARGS_TYPE info) {
	std::string name;

	// Parameters: <string> Name
	ASSERT_INFO_LENGTH(info, 1);
	ASSERT_GET_VALUE(info[0], name);

	// Perform the call
	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;

		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		// Results
		uint64_t sourceId;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->sourceId = rval[1].value_union.ui64;

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "Create",
		std::vector<ipc::value>{ ipc::value(name) }, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new Filter
	osn::Scene* obj = new osn::Scene(rtd.sourceId);
	info.GetReturnValue().Set(osn::Scene::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::CreatePrivate(Nan::NAN_METHOD_ARGS_TYPE info) {
	std::string name;

	// Parameters: <string> Name
	ASSERT_INFO_LENGTH(info, 1);
	ASSERT_GET_VALUE(info[0], name);

	// Perform the call
	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;

		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		// Results
		uint64_t sourceId;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->sourceId = rval[1].value_union.ui64;

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "CreatePrivate",
		std::vector<ipc::value>{ ipc::value(name) }, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new Filter
	osn::Scene* obj = new osn::Scene(rtd.sourceId);
	info.GetReturnValue().Set(osn::Scene::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::FromName(Nan::NAN_METHOD_ARGS_TYPE info) {
	std::string name;

	// Parameters: <string> Name
	ASSERT_INFO_LENGTH(info, 1);
	ASSERT_GET_VALUE(info[0], name);

	// Perform the call
	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;

		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		// Results
		uint64_t sourceId;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->sourceId = rval[1].value_union.ui64;

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "FromName",
	{ ipc::value(name) }, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new Filter
	osn::Scene* obj = new osn::Scene(rtd.sourceId);
	info.GetReturnValue().Set(osn::Scene::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Release(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* source = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), source)) {
		return;
	}

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "Release",
		std::vector<ipc::value>{ipc::value(source->sourceId)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	source->sourceId = UINT64_MAX;
	return;
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Remove(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* source = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), source)) {
		return;
	}

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "Remove",
		std::vector<ipc::value>{ipc::value(source->sourceId)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	source->sourceId = UINT64_MAX;
	return;
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::AsSource(Nan::NAN_METHOD_ARGS_TYPE info) {
	// Scenes are simply stored as a normal source object on the server, no additional calls necessary.
	osn::Scene* source = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), source)) {
		return;
	}

	// Create new Input
	osn::Input* obj = new osn::Input(source->sourceId);
	info.GetReturnValue().Set(osn::Input::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Duplicate(Nan::NAN_METHOD_ARGS_TYPE info) {
	std::string name;
	int duplicate_type;
	osn::Scene* source = nullptr;

	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), source)) {
		return;
	}

	ASSERT_INFO_LENGTH(info, 2);
	ASSERT_GET_VALUE(info[0], name);
	ASSERT_GET_VALUE(info[1], duplicate_type);

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		uint64_t source_id;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->source_id = rval[1].value_union.ui64;
		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "Duplicate",
		std::vector<ipc::value>{ipc::value(source->sourceId), ipc::value(name), ipc::value(duplicate_type)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new Scene
	osn::Scene* obj = new osn::Scene(rtd.source_id);
	info.GetReturnValue().Set(osn::Scene::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::AddSource(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	osn::Input* input = nullptr;
	ASSERT_INFO_LENGTH(info, 1);
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Input>(info[0]->ToObject(), input)) {
		return;
	}

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		uint64_t id;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->id = rval[1].value_union.ui64;
		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "AddSource",
		std::vector<ipc::value>{ipc::value(scene->sourceId), ipc::value(input->sourceId)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new SceneItem
	osn::SceneItem* obj = new osn::SceneItem(rtd.id);
	info.GetReturnValue().Set(osn::SceneItem::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::FindItem(Nan::NAN_METHOD_ARGS_TYPE info) {
	bool haveName = false;
	std::string name;
	int64_t position;

	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	ASSERT_INFO_LENGTH(info, 1);
	if (info[0]->IsNumber()) {
		haveName = false;
		ASSERT_GET_VALUE(info[0], position);
	} else if (info[0]->IsNumber()) {
		haveName = true;
		ASSERT_GET_VALUE(info[0], name);
	} else {
		Nan::TypeError("Expected string or number");
		return;
	}

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		uint64_t id;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->id = rval[1].value_union.ui64;
		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "FindItem",
		std::vector<ipc::value>{ipc::value(scene->sourceId), 
		(haveName ? ipc::value(name) : ipc::value(position))}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new SceneItem
	osn::SceneItem* obj = new osn::SceneItem(rtd.id);
	info.GetReturnValue().Set(osn::SceneItem::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::MoveItem(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	int from, to;
	ASSERT_INFO_LENGTH(info, 2);
	ASSERT_GET_VALUE(info[0], from);
	ASSERT_GET_VALUE(info[1], to);

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "MoveItem",
		std::vector<ipc::value>{ipc::value(scene->sourceId), ipc::value(from), ipc::value(to)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::GetItemAtIndex(Nan::NAN_METHOD_ARGS_TYPE info) {
	int64_t index;

	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	ASSERT_INFO_LENGTH(info, 1);
	ASSERT_GET_VALUE(info[0], index);

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		uint64_t id;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		rtd->id = rval[1].value_union.ui64;
		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "GetItem",
		std::vector<ipc::value>{ipc::value(scene->sourceId), ipc::value(index)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	// Create new SceneItem
	osn::SceneItem* obj = new osn::SceneItem(rtd.id);
	info.GetReturnValue().Set(osn::SceneItem::Store(obj));
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::GetItems(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		std::list<uint64_t> ids;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		for (size_t idx = 1; idx < rval.size(); idx++) {
			rtd->ids.push_back(rval[idx].value_union.ui64);
		}

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "GetItems",
		std::vector<ipc::value>{ipc::value(scene->sourceId)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	auto arr = Nan::New<v8::Array>(rtd.ids.size());
	auto iter = rtd.ids.begin();
	for (int idx = 0; idx < rtd.ids.size(); idx++) {
		osn::SceneItem* obj = new osn::SceneItem(*iter);
		Nan::Set(arr, idx, osn::SceneItem::Store(obj));
		iter++;
	}
	info.GetReturnValue().Set(arr);
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::GetItemsInRange(Nan::NAN_METHOD_ARGS_TYPE info) {
	osn::Scene* scene = nullptr;
	if (!utilv8::RetrieveDynamicCast<osn::ISource, osn::Scene>(info.This(), scene)) {
		return;
	}

	int32_t from, to;
	ASSERT_INFO_LENGTH(info, 2);
	ASSERT_GET_VALUE(info[0], from);
	ASSERT_GET_VALUE(info[1], to);

	struct ThreadData {
		std::condition_variable cv;
		std::mutex mtx;
		bool called = false;
		ErrorCode error_code = ErrorCode::Ok;
		std::string error_string = "";

		std::list<uint64_t> ids;
	} rtd;

	auto fnc = [](const void* data, const std::vector<ipc::value>& rval) {
		ThreadData* rtd = const_cast<ThreadData*>(static_cast<const ThreadData*>(data));

		if ((rval.size() == 1) && (rval[0].type == ipc::type::Null)) {
			rtd->error_code = ErrorCode::Error;
			rtd->error_string = rval[0].value_str;
			rtd->called = true;
			rtd->cv.notify_all();
			return;
		}

		rtd->error_code = (ErrorCode)rval[0].value_union.ui64;
		if (rtd->error_code != ErrorCode::Ok) {
			rtd->error_string = rval[1].value_str;
		}

		for (size_t idx = 1; idx < rval.size(); idx++) {
			rtd->ids.push_back(rval[idx].value_union.ui64);
		}

		rtd->called = true;
		rtd->cv.notify_all();
	};

	bool suc = Controller::GetInstance().GetConnection()->call("Scene", "GetItemsInRange",
		std::vector<ipc::value>{ipc::value(scene->sourceId), ipc::value(from), ipc::value(to)}, fnc, &rtd);
	if (!suc) {
		info.GetIsolate()->ThrowException(
			v8::Exception::Error(
				Nan::New<v8::String>(
					"Failed to make IPC call, verify IPC status."
					).ToLocalChecked()
			));
		return;
	}

	std::unique_lock<std::mutex> ulock(rtd.mtx);
	rtd.cv.wait(ulock, [&rtd]() { return rtd.called; });

	if (rtd.error_code != ErrorCode::Ok) {
		if (rtd.error_code == ErrorCode::InvalidReference) {
			info.GetIsolate()->ThrowException(
				v8::Exception::ReferenceError(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		} else {
			info.GetIsolate()->ThrowException(
				v8::Exception::Error(Nan::New<v8::String>(
					rtd.error_string).ToLocalChecked()));
		}
		return;
	}

	auto arr = Nan::New<v8::Array>(rtd.ids.size());
	auto iter = rtd.ids.begin();
	for (int idx = 0; idx < rtd.ids.size(); idx++) {
		osn::SceneItem* obj = new osn::SceneItem(*iter);
		Nan::Set(arr, idx, osn::SceneItem::Store(obj));
		iter++;
	}
}

/**
If libobs allowed us the ability to parse
or obtain info about the signals associated with
a handler, this could be done generically instead of a
hard coded table like this.

Notice that in this case, the signal handler of a scene
is in addition to the signals a source can receive.
However, I just require you use the signal handler
associated with the input object instead to keep things
simple.
*/
//static const char *signal_type_map[] = {
//	"item_add",
//	"item_remove",
//	"reorder",
//	"item_visible",
//	"item_select",
//	"item_deselect",
//	"item_transform"
//};
//
//enum signal_types {
//	SIG_ITEM_ADD,
//	SIG_ITEM_REMOVE,
//	SIG_REORDER,
//	SIG_ITEM_VISIBLE,
//	SIG_ITEM_SELECT,
//	SIG_ITEM_DESELECT,
//	SIG_ITEM_TRANSORM,
//	SIG_TYPE_OVERFLOW
//};
//
//static calldata_desc scene_signal_desc[] = {
//	{ "scene", CALLDATA_TYPE_SCENE },
//{ "", CALLDATA_TYPE_END }
//};
//
//static calldata_desc item_signal_desc[] = {
//	{ "scene", CALLDATA_TYPE_SCENE },
//{ "item", CALLDATA_TYPE_SCENEITEM },
//{ "", CALLDATA_TYPE_END }
//};
//
//static calldata_desc item_visible_signal_desc[] = {
//	{ "scene", CALLDATA_TYPE_SCENE },
//{ "item", CALLDATA_TYPE_SCENEITEM },
//{ "visibility", CALLDATA_TYPE_BOOL },
//{ "", CALLDATA_TYPE_END }
//};
//
//
//static calldata_desc *callback_desc_map[] = {
//	item_signal_desc,
//	item_signal_desc,
//	scene_signal_desc,
//	item_visible_signal_desc,
//	item_signal_desc,
//	item_signal_desc,
//	item_signal_desc
//};

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Connect(Nan::NAN_METHOD_ARGS_TYPE info) {
	//obs::weak<obs::scene> &scene = Scene::Object::GetHandle(info.Holder());
	//Scene* this_binding = Nan::ObjectWrap::Unwrap<Scene>(info.Holder());

	//uint32_t signal_type;
	//v8::Local<v8::Function> callback;

	//ASSERT_GET_VALUE(info[0], signal_type);
	//ASSERT_GET_VALUE(info[1], callback);

	//if (signal_type >= SIG_TYPE_OVERFLOW || signal_type < 0) {
	//	Nan::ThrowError("Detected signal type out of range");
	//	return;
	//}

	//SceneSignalCallback *cb_binding =
	//	new SceneSignalCallback(
	//		this_binding,
	//		CalldataEventHandler<Scene, callback_data, SceneSignalCallback>,
	//		callback);

	//cb_binding->user_data =
	//	callback_desc_map[signal_type];

	//scene.get()->connect(
	//	signal_type_map[signal_type],
	//	GenericSignalHandler<SceneSignalCallback>,
	//	cb_binding);

	//auto object = SceneSignalCallback::Object::GenerateObject(cb_binding);
	//cb_binding->obj_ref.Reset(object);
	//info.GetReturnValue().Set(object);
}

Nan::NAN_METHOD_RETURN_TYPE osn::Scene::Disconnect(Nan::NAN_METHOD_ARGS_TYPE info) {
	//obs::weak<obs::scene> &scene = Scene::Object::GetHandle(info.Holder());

	//uint32_t signal_type;
	//v8::Local<v8::Object> cb_data_object;

	//ASSERT_GET_VALUE(info[0], signal_type);
	//ASSERT_GET_VALUE(info[1], cb_data_object);

	//if (signal_type >= SIG_TYPE_OVERFLOW || signal_type < 0) {
	//	Nan::ThrowError("Detected signal type out of range");
	//	return;
	//}

	//SceneSignalCallback *cb_binding =
	//	SceneSignalCallback::Object::GetHandle(cb_data_object);

	//cb_binding->stopped = true;
	//cb_binding->obj_ref.Reset();

	//scene.get()->disconnect(
	//	signal_type_map[signal_type],
	//	GenericSignalHandler<SceneSignalCallback>,
	//	cb_binding);
}