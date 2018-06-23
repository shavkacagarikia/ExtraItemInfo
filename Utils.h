#pragma once
#include "f4se\PapyrusVM.h"
#include "f4se\PapyrusInterfaces.h"

template <typename T>
T GetVirtualFunction(void* baseObject, int vtblIndex) {
	uintptr_t* vtbl = reinterpret_cast<uintptr_t**>(baseObject)[0];
	return reinterpret_cast<T>(vtbl[vtblIndex]);
}

template <typename T>
T GetOffset(const void* baseObject, int offset) {
	return *reinterpret_cast<T*>((uintptr_t)baseObject + offset);
}

struct PropertyInfo {
	BSFixedString		scriptName;		// 00
	BSFixedString		propertyName;	// 08
	UInt64				unk10;			// 10
	void*				unk18;			// 18
	void*				unk20;			// 20
	void*				unk28;			// 28
	SInt32				index;			// 30	-1 if not found
	UInt32				unk34;			// 34
	BSFixedString		unk38;			// 38
};
STATIC_ASSERT(offsetof(PropertyInfo, index) == 0x30);
STATIC_ASSERT(sizeof(PropertyInfo) == 0x40);

typedef bool(*_GetPropertyValueByIndex)(VirtualMachine* vm, VMIdentifier** identifier, int idx, VMValue* outValue);
const int Idx_GetPropertyValueByIndex = 0x25;

typedef void* (*_GetPropertyInfo)(VMObjectTypeInfo* objectTypeInfo, void* outInfo, BSFixedString* propertyName, bool unk4);
RelocAddr <_GetPropertyInfo> GetPropertyInfo_Internal(0x027188E0);

typedef bool(*_IKeywordFormBase_HasKeyword)(IKeywordFormBase* keywordFormBase, BGSKeyword* keyword, UInt32 unk3);

TESForm * GetFormFromIdentifier(const std::string & identifier)
{
	auto delimiter = identifier.find('|');
	if (delimiter != std::string::npos) {
		std::string modName = identifier.substr(0, delimiter);
		std::string modForm = identifier.substr(delimiter + 1);

		const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName.c_str());
		if (mod && mod->modIndex != -1) {
			UInt32 formID = std::stoul(modForm, nullptr, 16) & 0xFFFFFF;
			UInt32 flags = GetOffset<UInt32>(mod, 0x334);
			if (flags & (1 << 9)) {
				// ESL
				formID &= 0xFFF;
				formID |= 0xFE << 24;
				formID |= GetOffset<UInt16>(mod, 0x372) << 12;	// ESL load order
			}
			else {
				formID |= (mod->modIndex) << 24;
			}
			return LookupFormByID(formID);
		}
	}
	return nullptr;
}

void GetPropertyInfo(VMObjectTypeInfo * objectTypeInfo, PropertyInfo * outInfo, BSFixedString * propertyName)
{
	GetPropertyInfo_Internal(objectTypeInfo, outInfo, propertyName, 1);
}

bool GetPropertyValue(TESObjectREFR* form, const char * propertyName, VMValue * valueOut)
{
	TESObjectREFR* targetForm = form;
	if (!targetForm) {
		_WARNING("Warning: Cannot retrieve property value %s from a None form. (%s)", propertyName, form);
		return false;
	}

	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
	VMValue targetScript;
	PackValue(&targetScript, &targetForm, vm);
	if (!targetScript.IsIdentifier()) {
		_WARNING("Warning: Cannot retrieve a property value %s from a form with no scripts attached. (%s)", propertyName, form);
		return false;
	}

	// Find the property
	PropertyInfo pInfo = {};
	pInfo.index = -1;
	GetPropertyInfo(targetScript.data.id->m_typeInfo, &pInfo, &BSFixedString(propertyName));
	BSFixedString* fs = targetScript.data.GetStr();
	if (pInfo.index != -1) {
		//vm->Unk_25(&targetScript.data.id, pInfo.index, valueOut);
		GetVirtualFunction<_GetPropertyValueByIndex>(vm, Idx_GetPropertyValueByIndex)(vm, &targetScript.data.id, pInfo.index, valueOut);
		return true;
	}
	else {
		_WARNING("Warning: Property %s does not exist on script %s", propertyName, targetScript.data.id->m_typeInfo->m_typeName.c_str());
		return false;
	}
}


//bool MCMUtils::SetPropertyValue(const char * formIdentifier, const char * propertyName, VMValue * valueIn)
//{
//	TESForm* targetForm = GetFormFromIdentifier(formIdentifier);
//	if (!targetForm) {
//		_WARNING("Warning: Cannot set property %s on a None form. (%s)", propertyName, formIdentifier);
//		return false;
//	}
//
//	VirtualMachine* vm = (*G::gameVM)->m_virtualMachine;
//	VMValue targetScript;
//	PackValue(&targetScript, &targetForm, vm);
//	if (!targetScript.IsIdentifier()) {
//		_WARNING("Warning: Cannot set a property value %s on a form with no scripts attached. (%s)", propertyName, formIdentifier);
//		return false;
//	}
//
//	UInt64 unk4 = 0;
//	//vm->Unk_23(&targetScript.data.id, propertyName, valueIn, &unk4);
//	GetVirtualFunction<_SetPropertyValue>(vm, Idx_SetPropertyValue)(vm, &targetScript.data.id, propertyName, valueIn, &unk4);
//
//	return true;
//}

bool HasKeyword(TESForm* form, BGSKeyword* keyword) {
	IKeywordFormBase* keywordFormBase = DYNAMIC_CAST(form, TESForm, IKeywordFormBase);
	if (keywordFormBase) {
		auto HasKeyword_Internal = GetVirtualFunction<_IKeywordFormBase_HasKeyword>(keywordFormBase, 1);
		if (HasKeyword_Internal(keywordFormBase, keyword, 0)) {
			return true;
		}
	}
	return false;
}

BSFixedString GetFormDescription(TESForm * thisForm)
{
	if (!thisForm)
		return BSFixedString();

	TESDescription * pDescription = DYNAMIC_CAST(thisForm, TESForm, TESDescription);
	if (pDescription) {
		BSString str;
		CALL_MEMBER_FN(pDescription, Get)(&str, nullptr);
		return str.Get();
	}

	return BSFixedString();
}

