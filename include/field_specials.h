#ifndef GUARD_FIELD_SPECIALS_H
#define GUARD_FIELD_SPECIALS_H

extern bool8 gBikeCyclingChallenge;
extern u8 gBikeCollisions;

u8 GetLeadMonIndex(void);
bool8 IsDestinationBoxFull(void);
u16 GetPCBoxToSendMon(void);
bool8 InMultiPartnerRoom(void);
void UpdateTrainerFansAfterLinkBattle(void);
void IncrementBirthIslandRockStepCount(void);
bool8 AbnormalWeatherHasExpired(void);
bool8 ShouldDoBrailleRegicePuzzle(void);

// PG3 quest: Eon Ticket / Lati awakening sidequest
u8 GetLatiPartyStatus(void);
bool32 ShouldLatiAwaken(void);
bool32 ShouldDoLatiBirchCall(void);
void DoLatiSpawnOutAnim(void);
void DoLatiSpawnInAnim(void);

// PG3 quest: Aurora Ticket / weather anomaly sidequest
bool8 IsRayquazaInParty(void);
bool32 ShouldDoWeatherInstituteCall(void);
bool32 ShouldDoStevenTicketCall(void);
bool8 IsQuestWeatherCoordEventSuppressed(void);

// PG3 Phase 2 prototype: gym leader rematch-tier gating
u8 GetRoxanneAllowedRematchTier(void);
bool32 ShouldDoRoxanneRematchCall(void);
u8 GetRoxanneNextRematchTier(void);
void SetRoxanneLastAcknowledgedRematchTier(void);
u8 GetBrawlyAllowedRematchTier(void);
bool32 ShouldDoBrawlyRematchCall(void);
u8 GetBrawlyNextRematchTier(void);
void SetBrawlyLastAcknowledgedRematchTier(void);
u8 GetWattsonAllowedRematchTier(void);
bool32 ShouldDoWattsonRematchCall(void);
u8 GetWattsonNextRematchTier(void);
void SetWattsonLastAcknowledgedRematchTier(void);
u8 GetFlanneryAllowedRematchTier(void);
bool32 ShouldDoFlanneryRematchCall(void);
u8 GetFlanneryNextRematchTier(void);
void SetFlanneryLastAcknowledgedRematchTier(void);
u8 GetNormanAllowedRematchTier(void);
bool32 ShouldDoNormanRematchCall(void);
u8 GetNormanNextRematchTier(void);
void SetNormanLastAcknowledgedRematchTier(void);
u8 GetWinonaAllowedRematchTier(void);
bool32 ShouldDoWinonaRematchCall(void);
u8 GetWinonaNextRematchTier(void);
void SetWinonaLastAcknowledgedRematchTier(void);
u8 GetTateAndLizaAllowedRematchTier(void);
bool32 ShouldDoTateAndLizaRematchCall(void);
u8 GetTateAndLizaNextRematchTier(void);
void SetTateAndLizaLastAcknowledgedRematchTier(void);
u8 GetJuanAllowedRematchTier(void);
bool32 ShouldDoJuanRematchCall(void);
u8 GetJuanNextRematchTier(void);
void SetJuanLastAcknowledgedRematchTier(void);
bool8 IsNormanRematchPending(void);

bool32 ShouldDoWallyCall(void);
bool32 ShouldDoScottFortreeCall(void);
bool32 ShouldDoScottBattleFrontierCall(void);
bool32 ShouldDoRoxanneCall(void);
bool32 ShouldDoRivalRayquazaCall(void);
bool32 CountSSTidalStep(u16 delta);
u8 GetSSTidalLocation(s8 *mapGroup, s8 *mapNum, s16 *x, s16 *y);
void ShowScrollableMultichoice(void);
void FrontierGamblerSetWonOrLost(bool8 won);
u8 TryGainNewFanFromCounter(u8 incrementId);
bool8 InPokemonCenter(void);
void SetShoalItemFlag(u16 unused);
void UpdateFrontierManiac(u16 daysSince);
void UpdateFrontierGambler(u16 daysSince);
void ResetCyclingRoadChallengeData(void);
bool8 UsedPokemonCenterWarp(void);
void ResetFanClub(void);
bool8 ShouldShowBoxWasFullMessage(void);
void SetPCBoxToSendMon(u8 boxId);

#endif // GUARD_FIELD_SPECIALS_H
