// WarioWare mod by Headshotnoby

#include <engine/shared/config.h>
#include "flood.h"

MGFlood::MGFlood(CGameContext* pGameServer, CGameControllerWarioWare* pController) : Microgame(pGameServer, pController)
{
	m_microgameName = "flood";
	m_boss = false;
}

void MGFlood::Start()
{
	for (int i=0; i<MAX_CLIENTS; i++)
	{
		if (not GameServer()->GetPlayerChar(i)) continue;

		Controller()->g_Complete[i] = true;
		Controller()->teleportPlayer(i, 5);
	}
	
	GameServer()->SendBroadcast("踩上平台吧！", -1);
	Controller()->setPlayerTimers(g_Config.m_WwSndMgGetOnPlatform_Offset, g_Config.m_WwSndMgGetOnPlatform_Length);
}

void MGFlood::End()
{
	for (int i=0; i<MAX_CLIENTS; i++)
	{
		if (not GameServer()->GetPlayerChar(i)) continue;

		Controller()->teleportPlayerToSpawn(i);
	}
}

void MGFlood::Tick()
{
	// nothing to tick
}

int MGFlood::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	Controller()->g_Complete[pVictim->GetPlayer()->GetCID()] = false;
	return 0;
}