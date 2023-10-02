#include <Windows.h>
#include "utility.h"
REL::Relocation<uintptr_t> ptr_DoHitMe{ REL::ID(1546751), 0x921 };
RE::ActorValueInfo* DamageResist;
RE::ActorValueInfo* RadResist;
RE::ActorValueInfo* EnergyResist;
RE::ActorValueInfo* CryoResist;
RE::ActorValueInfo* FireResist;
RE::ActorValueInfo* LightResist;
RE::BGSKeyword* ActorTypeHuman;
RE::BGSKeyword* ActorTypeGhoul;
RE::TESRace* HumanRace;
RE::BGSPerk* PowerArmorPerk;

const char* configFile = ".\\Data\\F4se\\Plugins\\DirectHit.ini";
const char* settingsSectionLog = "Log";
const char* settingsSection = "Settings";

bool powerArmorNegation = true;
bool underWearAppliesToOtherBodyParts = true;

struct damageType
{
	float damage;
	float resist;
};
namespace
{

	bool PowerArmorCheck(RE::Actor* victim) {
		//logger::debug("Power Armor Perk {:08X}", PowerArmorPerk->formID);
		
		if (powerArmorNegation && victim->HasPerk(PowerArmorPerk)) {
			//logger::debug("Power Armor Perk true");
			return true;
		} else {
			//logger::debug("Power Armor Perk false");
			return false;
		}
	}

	void modifyHitDataForPowerArmorAbsorb(RE::HitData& hitData) {
		hitData.calculatedBaseDamage = 0;
		logger::debug("Power Armor Hit");
	}

	RE::Setting* GetGameSetting(const char* a_name)
	{
		const auto gsc = RE::GameSettingCollection::GetSingleton();
		const auto it = gsc->settings.find(a_name);
		return it != gsc->settings.end() ? it->second : nullptr;
	}

	float GetDamageTypeValue(vector<RE::TESObjectARMO::InstanceData*> armorData, uint32_t formID, float armorNatural)
	{
		float value = 0;

		for (size_t i = 0; i < armorData.size(); i++) {
			if (armorData[i] && armorData[i]->damageTypes && !armorData[i]->damageTypes[0].empty()) {
				for (size_t j = 0; j < armorData[i]->damageTypes[0].size(); j++) {
					if (armorData[i]->damageTypes[0][j].first && armorData[i]->damageTypes[0][j].first->formID == formID) {
						logger::debug("*** GetDamageTypeResist: {:08X} {} += {}", armorData[i]->damageTypes[0][j].first->formID, value, armorData[i]->damageTypes[0][j].second.i);
						value += armorData[i]->damageTypes[0][j].second.i;
					}
				}
			}
		}
		logger::debug("*** Total DamageTypeResist {:08X} from armor {}", formID, value);
		value = value + armorNatural;
		return value;
	}

	float GetDamageResist(vector<RE::TESObjectARMO::InstanceData*> armorData, float naturalArmor)
	{
		float value = 0;
		for (size_t i = 0; i < armorData.size(); i++) {
			logger::debug("### GetDamageResist: {} += {}", value, armorData[i]->rating);
			value = value + armorData[i]->rating;
		}
		logger::debug("### Total DamageResist from armor {}", value);
		value = value + naturalArmor;
		return value;
	}

	struct MyHook
	{
		static void HookedDoHitMe(RE::Actor* a, RE::HitData& hitData)
		{
			//hitData.damageTypes[0][0].first->As<RE::BGSDamageType>()->data.resistance;


			int8_t numProjectiles = 1;
			auto victim = hitData.victimHandle.get().get()->As<RE::Actor>();

			if (victim->IsDead(false)) {
				logger::debug("Actor is dead!");

				return BlockOrg(a, hitData);
			}

			//Power Armor
			

			//

			if (hitData.attackerHandle && hitData.attackerHandle.get() && hitData.attackerHandle.get().get()) {
				auto object = hitData.attackerHandle.get().get();
				if (object) {
					auto actor = object->As<RE::Actor>();
					if (actor) {
						if (actor->currentProcess->middleHigh->equippedItems.size() > 0) {
							actor->currentProcess->middleHigh->equippedItemsLock.lock();
							RE::EquippedItem& equipped = actor->currentProcess->middleHigh->equippedItems[0];
							RE::TESObjectWEAP* wepForm = (RE::TESObjectWEAP*)equipped.item.object;
							RE::TESObjectWEAP::InstanceData* instance = (RE::TESObjectWEAP::InstanceData*)equipped.item.instanceData.get();
							//RE::EquippedWeaponData* wepData = (RE::EquippedWeaponData*)equipped.data.get();
							actor->currentProcess->middleHigh->equippedItemsLock.unlock();
							if (instance && instance->rangedData) {
								numProjectiles = instance->rangedData->numProjectiles;
							} else if (wepForm && wepForm->weaponData.rangedData) {
								numProjectiles = wepForm->weaponData.rangedData->numProjectiles;
							}
						}
					}
				}
			}

			char* testt = nullptr;

			/*			logger::debug("Source {:08X} {:08X}", weapon->formID, weapon->data.objectReference->formID);
			if (weapon) {
				logger::debug("weapon");
				auto instanceData = (RE::TBO_InstanceData*)weapon->extraList->GetInstanceData();
				if (instanceData) {
					logger::debug("InstanceData");
					RE::TESObjectWEAP::InstanceData* weaponData = (RE::TESObjectWEAP::InstanceData*)instanceData;

					if (weaponData) {
						numProjectiles = weaponData->rangedData->numProjectiles;
						logger::debug("weaponData->proj {}", weaponData->rangedData->numProjectiles);
					}
				} else if (weapon->data.objectReference) {
					numProjectiles = weapon->data.objectReference->As<RE::TESObjectWEAP>()->weaponData.rangedData->numProjectiles;
					logger::debug("weapon->objectRef  numProjectiles {}", weapon->data.objectReference->As<RE::TESObjectWEAP>()->weaponData.rangedData->numProjectiles);
				}
			}*/

			if (!a) {
				logger::debug("Source is not a actor!");
			}

			if (hitData.source.object && !hitData.source.object->IsWeapon()) {
				logger::debug("Source is not a weapon!");
			}

			//if (!hitData.victimHandle.get().get()->HasKeyword(ActorTypeGhoul) && !hitData.victimHandle.get().get()->HasKeyword(ActorTypeHuman)) {
			//	BlockOrg(a, hitData);
			//}

			damageType dt{ 0, 0 };
			logger::debug("Damage values before recalculation");
			logger::debug("******************************");
			logger::debug("bodyPart Hit {} ", hitData.bodypartType);
			logger::debug("armorBaseFactorSum {} ", victim->armorBaseFactorSum);
			logger::debug("totalDamage {}", hitData.totalDamage);
			logger::debug("reducedDamage {}", hitData.reducedDamage);
			logger::debug("baseDamage {}", hitData.baseDamage);
			logger::debug("reducedDamageTypes {}", hitData.reducedDamageTypes);
			logger::debug("calculatedBaseDamage {}", hitData.calculatedBaseDamage);
			logger::debug("******************************");
			//if (hitData.attackData && hitData.attackData->data.attackType->formID)
			//	logger::debug("attackType {}", hitData.attackData->data.attackType->formID);
			//hitData.calculatedBaseDamage = 250;

			//Init

			//RE::BSTArray newList = efL->data.operator=;
			RE::BSTArray<RE::BSTSmartPointer<RE::ActiveEffect>> effectList;
			effectList = victim->currentProcess->middleHigh->activeEffects.data;

			//if (effectList.size() > 0) {
			//	for (auto& ae : effectList) {
			//		if (ae.get()->effect->effectSetting->data.primaryAV && !(ae.get()->flags & RE::ActiveEffect::Flags::kInactive) && ae.get()->effect->effectSetting->data.archetype == RE::EffectArchetypes::ArchetypeID::kPeakValueModifier) {
			//			logger::debug("active Effect {:08x} av {:08x} mag {} at {}", ae.get()->effect->effectSetting->formID, ae.get()->effect->effectSetting->data.primaryAV->formID, ae.get()->magnitude, ae.get()->effect->effectSetting->data.archetype.underlying());
			//		}
			//	}
			//}
			bool isequipped = false; 
			float armor = 0;
			float armorNatural = 0;
			armorNatural = hitData.victimHandle.get().get()->data.objectReference->As<RE::TESNPC>()->GetActorValue(*DamageResist);
			float secondDamage = 0;
			float bodyPartMult = hitData.calculatedBaseDamage / (hitData.baseDamage - hitData.reducedDamage);
			vector<RE::TESObjectARMO::InstanceData*> armorData;


			switch (hitData.bodypartType) {
			case 0:
				if (victim->biped->object[11].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[11].parent.instanceData.get()) {  //Torso
					logger::debug(FMT_STRING("-> Torso Armor {:08X} {}"), victim->biped->object[11].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[11].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[11].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}

				}
				if (victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
					logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
				}
				break;
			case 1:
				if (victim->biped->object[0].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[0].parent.instanceData.get()) {  //Head
					logger::debug(FMT_STRING("-> Head Armor {:08X} {}"), victim->biped->object[0].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[0].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[0].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}
					if (underWearAppliesToOtherBodyParts && victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
						logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
						armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
					}
				}
				//RE::PowerArmor::IsLimbCoveredByPowerArmor(*victim, RE::BGSBodyPartData::Head1);
				break;
			case 6:
				if (victim->biped->object[12].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[12].parent.instanceData.get()) {  // left arm
					logger::debug(FMT_STRING("-> Left Arm Armor {:08X} {}"), victim->biped->object[12].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[12].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[12].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}
					if (underWearAppliesToOtherBodyParts && victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
						logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
						armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
					}
				}
				//RE::PowerArmor::IsLimbCoveredByPowerArmor(*victim, RE::BGSBodyPartData::LeftArm1);
				break;
			case 8:
				if (victim->biped->object[13].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[13].parent.instanceData.get()) {  // right arm
					logger::debug(FMT_STRING("-> Right Arm Armor {:08X} {}"), victim->biped->object[13].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[13].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[13].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}
					if (underWearAppliesToOtherBodyParts && victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
						logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
						armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
					}
				}
				//RE::PowerArmor::IsLimbCoveredByPowerArmor(*victim, RE::BGSBodyPartData::RightArm1);
				break;
			case 10:
				if (victim->biped->object[14].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[14].parent.instanceData.get()) {  // left leg
					logger::debug(FMT_STRING("-> Left Leg Armor {:08X} {}"), victim->biped->object[14].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[14].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[14].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}
					if (underWearAppliesToOtherBodyParts && victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
						logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
						armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
					}
				}
				break;
			case 13:
				if (victim->biped->object[15].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[15].parent.instanceData.get()) {  // right leg
					logger::debug(FMT_STRING("-> Right Leg Armor {:08X} {}"), victim->biped->object[15].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[15].parent.instanceData.get())->rating);
					armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[15].parent.instanceData.get()));
					if (PowerArmorCheck(victim)) {
						modifyHitDataForPowerArmorAbsorb(hitData);
						return BlockOrg(a, hitData);
					}
					if (underWearAppliesToOtherBodyParts && victim->biped->object[3].parent.object && (RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()) {
						logger::debug(FMT_STRING("-> Torso Body {:08X} {}"), victim->biped->object[3].parent.object->formID, ((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get())->rating);
						armorData.push_back(((RE::TESObjectARMO::InstanceData*)victim->biped->object[3].parent.instanceData.get()));
					}
				}
				break;

			default:
				logger::debug("----> Return, unkown bodyPart!");
				logger::debug("---------");
				if (PowerArmorCheck(victim)) {
					modifyHitDataForPowerArmorAbsorb(hitData);
					return BlockOrg(a, hitData);
				}
				return BlockOrg(a, hitData);
			}

			//clear
			hitData.calculatedBaseDamage = 0;

			if (armorData.size() > 0) {
				if (hitData.damageTypes) {
					for (size_t i = 0; i < hitData.damageTypes[0].size(); i++) {
						logger::debug("*** DamageTypes Attack Damage {} {:08X} {}", i, hitData.damageTypes[0][i].first->formID, hitData.damageTypes[0][i].second.f);
						//Get Damage
						dt.damage = hitData.damageTypes[0][i].second.f;
						//Calc Resist
						float effectDT = 0;
						if (effectList.size() > 0) {
							for (auto& ae : effectList) {
								if (ae.get()->effect->effectSetting->data.primaryAV && ae.get()->effect->effectSetting->data.primaryAV->formID == hitData.damageTypes[0][i].first->As<RE::BGSDamageType>()->data.resistance->formID && !(ae.get()->flags & RE::ActiveEffect::Flags::kInactive) && ae.get()->effect->effectSetting->data.archetype == RE::EffectArchetypes::ArchetypeID::kPeakValueModifier) {
									effectDT = effectDT + ae.get()->magnitude;
									logger::debug("+++ active Effect Damage Type {:08x} av {:08x} mag {} at {}", ae.get()->effect->effectSetting->formID, ae.get()->effect->effectSetting->data.primaryAV->formID, ae.get()->magnitude, ae.get()->effect->effectSetting->data.archetype.underlying());
								}
							}
						}
						dt.resist = RE::CombatFormulas::calcResistedPercentage(hitData.damageTypes[0][i].first->As<RE::BGSDamageType>()->data.resistance, dt.damage * numProjectiles, GetDamageTypeValue(armorData, hitData.damageTypes[0][i].first->formID, (effectDT + victim->GetBaseActorValue(*hitData.damageTypes[0][i].first->As<RE::BGSDamageType>()->data.resistance))));

						hitData.calculatedBaseDamage = hitData.calculatedBaseDamage + dt.damage * dt.resist * bodyPartMult;
						logger::debug("*** DamageTypes calculatedBaseDamage: {} += Damage:{}* ProjCount:{} * Resist:{} * BPM:{} ", hitData.calculatedBaseDamage, dt.damage, numProjectiles, dt.resist, bodyPartMult);
					}
				}
			} else {
				logger::debug("No ArmorData.");
			}

			if (effectList.size() > 0) {
				for (auto& ae : effectList) {
					if (ae.get()->effect->effectSetting->data.primaryAV && ae.get()->effect->effectSetting->data.primaryAV->formID == DamageResist->formID && !(ae.get()->flags & RE::ActiveEffect::Flags::kInactive) && ae.get()->effect->effectSetting->data.archetype == RE::EffectArchetypes::ArchetypeID::kPeakValueModifier) {
						armorNatural = armorNatural + ae.get()->magnitude;
						logger::debug("+++ active Effect Damage Resist {:08x} av {:08x} mag {} at {}", ae.get()->effect->effectSetting->formID, ae.get()->effect->effectSetting->data.primaryAV->formID, ae.get()->magnitude, ae.get()->effect->effectSetting->data.archetype.underlying());
					}
				}
			}
			
			float summedArmor = GetDamageResist(armorData, armorNatural);
			logger::debug("### summed DR: {} included natural DR of that {}", summedArmor, armorNatural);

			if (hitData.attackerHandle && hitData.attackerHandle.get().get() && hitData.attackerHandle.get().get()->As<RE::Actor>())
				RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT::kModTargetDamageResistance, hitData.attackerHandle.get().get()->As<RE::Actor>(), hitData.source, &testt, &summedArmor);

			logger::debug("### summed DR after perks applied: {}", summedArmor);

			float phyDamage = hitData.totalDamage + hitData.reducedDamage;
			armor = RE::CombatFormulas::calcResistedPercentage(DamageResist, phyDamage * numProjectiles, summedArmor);
			logger::debug("### physical calculatedBaseDamage: {} = Damage:{}* ProjCount:{} * Resist:{} * BPM:{}", phyDamage * bodyPartMult * armor, phyDamage, numProjectiles, armor, bodyPartMult);
			hitData.calculatedBaseDamage = hitData.calculatedBaseDamage + phyDamage * bodyPartMult * armor;
			//logger::debug("damage before perk: {}", hitData.calculatedBaseDamage);
			//RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT::kModIncomingDamage, victim, &hitData.source, &testt, &hitData.calculatedBaseDamage);
			//RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT::kModIncomingDamage, victim, &hitData.source, &testt, &hitData.totalDamage);
			//logger::debug("damage after perk: {}", hitData.calculatedBaseDamage);

			float vatsmult = 1;
			RE::BSFixedString sVats = RE::BSFixedString("VATSMenu");
			RE::UI* uiInstance = RE::UI::GetSingleton();  // Get the singleton instance of the UI class
			if (uiInstance) {
				bool isMenuOpen = uiInstance->GetMenuOpen(sVats);
				if (isMenuOpen) {
					auto vatsSetting = GetGameSetting("fVATSPlayerDamageMult");
					vatsmult = vatsSetting->GetFloat();
				}
			}


			if (victim->formID == RE::PlayerCharacter::GetSingleton()->formID && (hitData.flags & 0x4000) == 0) {
				hitData.calculatedBaseDamage = hitData.calculatedBaseDamage * vatsmult;
				hitData.totalDamage = hitData.totalDamage * vatsmult;
				hitData.baseDamage = hitData.baseDamage * vatsmult;
				hitData.blockedDamage = hitData.blockedDamage * vatsmult;
				hitData.reducedDamage = hitData.reducedDamage * vatsmult;
				hitData.reducedDamageTypes = hitData.reducedDamageTypes * vatsmult;
				logger::debug("= final calculatedBaseDamage: {} * VatsMult:{}", hitData.calculatedBaseDamage, vatsmult);
			} else {
				logger::debug("= final calculatedBaseDamage: {}", hitData.calculatedBaseDamage);
			}
			logger::debug("---------");
			BlockOrg(a, hitData);
		}

		static inline REL::Relocation<decltype(HookedDoHitMe)> BlockOrg;

		static void Install()
		{
			if (GetPrivateProfileInt(settingsSection, "iEnable", 1, configFile)) {
				auto& trampoline = F4SE::GetTrampoline();
				trampoline.create(64);
				typedef float (*Block)(RE::Actor * a, RE::HitData & hitData);
				REL::Relocation<Block> blockCallLoc{ REL::ID(1546751), 0x921 };
				BlockOrg = trampoline.write_call<5>(blockCallLoc.address(), MyHook::HookedDoHitMe);
				logger::info("Hook installed."sv);
			} else {
				logger::info("Hook not installed, mod disabled by ini."sv);
			}
		}
	};

	void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}

		switch (a_msg->type) {
		case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					RE::TESForm* currentform = nullptr;
					std::string string_form = "Fallout4.esm|2E3";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						DamageResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2EA";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						RadResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2EB";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						EnergyResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2E5";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						FireResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2E7";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						CryoResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2E6";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kAVIF) {
						LightResist = (RE::ActorValueInfo*)currentform;
					} else {
						logger::critical("ActorValueInfo not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|2CB72";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kKYWD) {
						ActorTypeHuman = (RE::BGSKeyword*)currentform;
					} else {
						logger::critical("BGSKeyword not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|000EAFB7";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kKYWD) {
						ActorTypeGhoul = (RE::BGSKeyword*)currentform;
					} else {
						logger::critical("BGSKeyword not found."sv);
					}

					currentform = nullptr;
					string_form = "Fallout4.esm|0001F8A9";
					currentform = GetFormFromIdentifier(string_form);
					if (currentform && currentform->formType == RE::ENUM_FORM_ID::kPERK) {
						PowerArmorPerk = (RE::BGSPerk*)currentform;
					} else {
						logger::critical("BGSKeyword not found."sv);
					}
					
				}
			}
		}
	}

}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	if (GetPrivateProfileInt(settingsSectionLog, "iEnablelog", 0, configFile)) {
		log->set_level(spdlog::level::debug);
		log->flush_on(spdlog::level::debug);
	} else {
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
	}
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("(%#)[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	MyHook::Install();
	F4SE::GetMessagingInterface()->RegisterListener(MessageHandler);

	if (GetPrivateProfileInt(settingsSection, "iPowerArmorNegation", 1, configFile)) {
		powerArmorNegation = true;
		logger::debug("Power Armor Negation enabled");
	} else {
		powerArmorNegation = false;
		logger::debug("Power Armor Negation disabled");
	}

	if (GetPrivateProfileInt(settingsSection, "iUnderWearAppliesToOtherBodyParts", 1, configFile)) {
		underWearAppliesToOtherBodyParts = true;
		logger::debug("Apply Underwear to body parts enabled");
	} else {
		underWearAppliesToOtherBodyParts = false;
		logger::debug("Apply Underwear to body parts enabled");
	}

	return true;
}
