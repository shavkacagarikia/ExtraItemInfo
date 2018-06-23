#pragma once
#include "F4SE_common/F4SE_version.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameMenus.h"
#include "f4se/GameForms.h"
#include "f4se/GameObjects.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/GameExtraData.h"
#include "f4se\GameRTTI.h"
#include <shlobj.h>
#include <memory>
#include <string>

#include "HookUtil.h"
#include "Globals.h"



template <typename T>
T GetOffset1(const void* baseObject, int offset) {
	return *reinterpret_cast<T*>((uintptr_t)baseObject + offset);
}
TESForm * GetFormFromIdentifier1(const std::string & identifier)
{
	auto delimiter = identifier.find('|');
	if (delimiter != std::string::npos) {
		std::string modName = identifier.substr(0, delimiter);
		std::string modForm = identifier.substr(delimiter + 1);

		const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName.c_str());
		if (mod && mod->modIndex != -1) {
			UInt32 formID = std::stoul(modForm, nullptr, 16) & 0xFFFFFF;
			UInt32 flags = GetOffset1<UInt32>(mod, 0x334);
			if (flags & (1 << 9)) {
				// ESL
				formID &= 0xFFF;
				formID |= 0xFE << 24;
				formID |= GetOffset1<UInt16>(mod, 0x372) << 12;	// ESL load order
			}
			else {
				formID |= (mod->modIndex) << 24;
			}
			return LookupFormByID(formID);
		}
	}
	return nullptr;
}

#pragma once
template<typename T>
inline void Register(GFxValue * dst, const char * name, T value)
{

}

template<>
inline void Register(GFxValue * dst, const char * name, SInt32 value)
{
	GFxValue	fxValue;
	fxValue.SetInt(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, UInt32 value)
{
	GFxValue	fxValue;
	fxValue.SetUInt(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, double value)
{
	GFxValue	fxValue;
	fxValue.SetNumber(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, bool value)
{
	GFxValue	fxValue;
	fxValue.SetBool(value);
	dst->SetMember(name, &fxValue);
}

inline void RegisterString(GFxValue * dst, GFxMovieRoot * view, const char * name, const char * str)
{
	GFxValue	fxValue;
	view->CreateString(&fxValue, str);
	dst->SetMember(name, &fxValue);
}




class ItemMenuDataManager
{
public:

	DEFINE_MEMBER_FUNCTION(GetSelectedForm, TESForm *, 0x1A3740, UInt32 & handleID);
	DEFINE_MEMBER_FUNCTION(GetSelectedItem, BGSInventoryItem *, 0x1A3650, UInt32 & handleID);
	//BGSInventoryItem
};

RelocPtr <ItemMenuDataManager *> g_itemMenuDataMgr(0x590DA00); //48 8B 0D ? ? ? ? 48 8D 54 24 ? 89 44 24 60 Address moved in to RCX.


class PipboyDataManager
{
public:
	//4B8
	UInt64							unk00[0x4A8 >> 3];
	tArray<PipboyObject*>			itemData;
};
STATIC_ASSERT(sizeof(PipboyDataManager) == 0x4C0);
RelocPtr <PipboyDataManager *> g_pipboyDataMgr(0x5909B70);


bool hasHealth = false;
class PipboyMenu : public IMenu
{
public:
	class ScaleformArgs
	{
	public:

		GFxValue * result;	// 00
		GFxMovieView	* movie;	// 08
		GFxValue		* thisObj;	// 10
		GFxValue		* unk18;	// 18
		GFxValue		* args;		// 20
		UInt32			numArgs;	// 28
		UInt32			pad2C;		// 2C
		UInt32			optionID;	// 30 pUserData
	};

	using FnInvoke = void(__thiscall PipboyMenu::*)(ScaleformArgs *);
	static FnInvoke Invoke_Original;

	bool CreateItemData(PipboyMenu::ScaleformArgs * args, std::string text, std::string value) {

		if (!args) {
			return false;
		}

		auto * movieRoot = args->movie->movieRoot;
		auto & pInfoObj = args->args[1];
		GFxValue extraData;
		movieRoot->CreateObject(&extraData);
		RegisterString(&extraData, movieRoot, "text", text.c_str());//
		RegisterString(&extraData, movieRoot, "value", value.c_str());
		Register<SInt32>(&extraData, "difference", 0);
		args->args[1].PushBack(&extraData);
		return true;
	}


	void Invoke_Hook(ScaleformArgs * args)
	{
		(this->*Invoke_Original)(args);

		if (args->optionID == 0xD && args->numArgs == 4 && args->args[0].GetType() == GFxValue::kType_Int \
			&& args->args[1].GetType() == GFxValue::kType_Array && args->args[2].GetType() == GFxValue::kType_Array)
		{
			SInt32 selectedIndex = args->args[0].GetInt();
			PipboyObject * pHandlerData = nullptr;
			if (selectedIndex >= 0 && selectedIndex < (*g_pipboyDataMgr)->itemData.count)
			{
				pHandlerData = (*g_pipboyDataMgr)->itemData[selectedIndex];
			}
			if (pHandlerData != nullptr)
			{
				//static RelocPtr<BSFixedString>	memberName(0x5AA13C8);//handleID
				static RelocPtr<BSFixedString>	memberName(0x5AA23C8);//handleID

				auto * pipboyValue = static_cast<PipboyPrimitiveValue<UInt32>*>(pHandlerData->GetMemberValue(memberName));
				if (pipboyValue != nullptr)
				{
					UInt32 handleID = pipboyValue->value;
					auto * pSelectedData = (*g_itemMenuDataMgr)->GetSelectedItem(handleID);

					if (pSelectedData != nullptr)
					{
						TBO_InstanceData* neededInst = nullptr;

						TESForm * pSelectedForm = pSelectedData->form;
						ExtraDataList* extraDataList = pSelectedData->stack->extraData;
						if (extraDataList) {
							if (extraDataList->GetByType(ExtraDataType::kExtraData_Health)) {
								hasHealth = true;
							}
							BSExtraData * extraData = extraDataList->GetByType(ExtraDataType::kExtraData_InstanceData);
							if (extraData) {
								ExtraInstanceData * objectModData = DYNAMIC_CAST(extraData, BSExtraData, ExtraInstanceData);
								if (objectModData)
									neededInst = objectModData->instanceData;
							}
							else {

							}
						}
						switch (pSelectedForm->formType)
						{
						case kFormType_WEAP:
						{
							auto * pWeapon = (TESObjectWEAP::InstanceData*)Runtime_DynamicCast(neededInst, RTTI_TBO_InstanceData, RTTI_TESObjectWEAP__InstanceData);

							if (pWeapon) {
								UInt64	unk18 = pWeapon->unk18;
								UInt64	unk20 = pWeapon->unk20;
								UInt64	unk50 = pWeapon->unk50;
								float	unk20a = pWeapon->firingData->unk00;
								float	unk18a = pWeapon->firingData->unk18;
								float	unk1c = pWeapon->firingData->unk1C;
								UInt32 unk28 = pWeapon->firingData->unk28;
								float	unkC0 = pWeapon->unkC0;
								float	unkD8 = pWeapon->unkD8;
								float	unkEC = pWeapon->unkEC;
								UInt32	unk100 = pWeapon->unk100;
								UInt32	unk114 = pWeapon->unk114;
								UInt32	unk118 = pWeapon->unk118;
								UInt32	unk11C = pWeapon->unk11C;
								

								if (IsCapacityVisible) {
									if (pWeapon->ammoCapacity) {
										UInt16 aCap = pWeapon->ammoCapacity;
										std::string displayString = std::to_string(aCap);

										CreateItemData(args, "Capacity", displayString);
									}
								}
								if (IsAPCostVisible) {
									if (pWeapon->actionCost) {
										float apCost = pWeapon->actionCost;
										int rounded = round(apCost);
										std::string displayString = std::to_string(rounded);

										CreateItemData(args, "VATS AP Cost", displayString);
									}
								}
								if (IsWeapVWVisible) {
									if (pWeapon->value && pWeapon->weight) {
										float weight = pWeapon->weight;
										float value = pWeapon->value;
										float ratio = value / weight;

										std::string displayString = std::to_string(ratio);
										std::string befStr = displayString.substr(0, displayString.find(".") + 3);

										CreateItemData(args, "Val/Wt", befStr);
									}
								}
							}
							else {
								auto * fSelectedData = (*g_itemMenuDataMgr)->GetSelectedForm(handleID);
								auto * fWeapon = (TESObjectWEAP*)Runtime_DynamicCast(fSelectedData, RTTI_TESForm, RTTI_TESObjectWEAP);

								if (fWeapon) {
									if (IsCapacityVisible) {
										if (fWeapon->weapData.ammoCapacity) {
											UInt16 aCap = fWeapon->weapData.ammoCapacity;
											std::string displayString = std::to_string(aCap);

											CreateItemData(args, "Capacity", displayString);
										}
									}
									if (IsAPCostVisible) {
										if (fWeapon->weapData.actionCost) {
											float apCost = fWeapon->weapData.actionCost;
											int rounded = round(apCost);
											std::string displayString = std::to_string(rounded);

											CreateItemData(args, "VATS AP Cost", displayString);
										}
									}
									if (IsWeapVWVisible) {
										if (fWeapon->weapData.value && fWeapon->weapData.weight) {
											float weight = fWeapon->weapData.weight;
											float value = fWeapon->weapData.value;
											float ratio = value / weight;

											std::string displayString = std::to_string(ratio);
											std::string befStr = displayString.substr(0, displayString.find(".") + 3);

											CreateItemData(args, "Val/Wt", befStr);
										}
									}
								}
							}


							break;
						}
						case kFormType_ARMO:
						{
							if (IsArmoVWVisible) {
								auto * pArmor = (TESObjectARMO::InstanceData*)Runtime_DynamicCast(neededInst, RTTI_TBO_InstanceData, RTTI_TESObjectARMO__InstanceData);
								if (pArmor) {
									if (pArmor->value && pArmor->weight) {
										float weight = pArmor->weight;
										float value = pArmor->value;
										float ratio = value / weight;

										std::string displayString = std::to_string(ratio);
										std::string befStr = displayString.substr(0, displayString.find(".") + 3);

										CreateItemData(args, "Val/Wt", befStr);
									}
								}
								else {
									auto * fSelectedData = (*g_itemMenuDataMgr)->GetSelectedForm(handleID);
									auto * fArmor = (TESObjectARMO*)Runtime_DynamicCast(fSelectedData, RTTI_TESForm, RTTI_TESObjectARMO);

									if (fArmor) {
										if (fArmor->instanceData.value && fArmor->instanceData.weight) {
											float weight = fArmor->instanceData.weight;
											float value = fArmor->instanceData.value;
											float ratio = value / weight;

											std::string displayString = std::to_string(ratio);
											std::string befStr = displayString.substr(0, displayString.find(".") + 3);

											CreateItemData(args, "Val/Wt", befStr);
										}
										if (fArmor->bipedObject.data.parts) {

										}
									}

								}
							}
							break;
						}
						case kFormType_ALCH:
						{
							if (IsAlchVWVisible) {
								auto * fSelectedData = (*g_itemMenuDataMgr)->GetSelectedForm(handleID);
								auto * fPotion = (AlchemyItem*)Runtime_DynamicCast(fSelectedData, RTTI_TESForm, RTTI_AlchemyItem);

								if (fPotion) {
									if (fPotion->weightForm.weight) {
										float weight = fPotion->weightForm.weight;
										float value1 = fPotion->unk1A8;
										float value2 = fPotion->unk1AC;
										float ratio = value1 / weight;

										std::string displayString = std::to_string(ratio);
										std::string befStr = displayString.substr(0, displayString.find(".") + 3);

										CreateItemData(args, "Val/Wt", befStr);
									}
								}
							}
							break;
						}
						case kFormType_MISC:
						{
							if (IsMiscVWVisible) {
								auto * fSelectedData = (*g_itemMenuDataMgr)->GetSelectedForm(handleID);
								auto * fMisc = (TESObjectMISC*)Runtime_DynamicCast(fSelectedData, RTTI_TESForm, RTTI_TESObjectMISC);

								if (fMisc) {
									if (fMisc->value.value && fMisc->weight.weight) {
										float weight = fMisc->weight.weight;
										float value = fMisc->value.value;
										float ratio = value / weight;

										std::string displayString = std::to_string(ratio);
										std::string befStr = displayString.substr(0, displayString.find(".") + 3);
										CreateItemData(args, "Val/Wt", befStr);
									}
								}
							}
							break;
						}
						case kFormType_AMMO:
						{
							if (IsAmmoVWVisible) {
								auto * fSelectedData = (*g_itemMenuDataMgr)->GetSelectedForm(handleID);
								auto * fAmmo = (TESAmmo*)Runtime_DynamicCast(fSelectedData, RTTI_TESForm, RTTI_TESAmmo);

								if (fAmmo) {
									if (fAmmo->value.value && fAmmo->weight.weight) {
										float weight = fAmmo->weight.weight;
										float value = fAmmo->value.value;
										float ratio = value / weight;

										std::string displayString = std::to_string(ratio);
										std::string befStr = displayString.substr(0, displayString.find(".") + 3);
										CreateItemData(args, "Val/Wt", befStr);
									}
								}
							}
							break;
						}
						}

					}
				}
			}
		}
	}

	static void InitHooks()
	{
		Invoke_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D4CAE8) + 1 * 0x8, &Invoke_Hook);
	}
};
PipboyMenu::FnInvoke	PipboyMenu::Invoke_Original = nullptr;