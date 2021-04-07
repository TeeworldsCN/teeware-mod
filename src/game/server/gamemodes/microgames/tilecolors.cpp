// WarioWare mod by Headshotnoby

#include <engine/shared/config.h>
#include "tilecolors.h"

const char *colorNames[4][5] = {
	{"大红色", "玫瑰色", "榴弹色", "樱桃色", "大鬼色"},
	{"葱绿色", "草地色", "树叶色", "竹子色", "青蛙色"},
	{"天蓝色", "大海色", "泳池色", "蓝莓色", "三精色"},
	{"蛋黄色", "太阳色", "小鸭色", "玉米色", "香蕉色"}
};


MGTileColors::MGTileColors(CGameContext* pGameServer, CGameControllerWarioWare* pController) : Microgame(pGameServer, pController)
{
	m_microgameName = "tilecolors";
	m_boss = false;
}

void MGTileColors::Start()
{
	for (int i=0; i<MAX_CLIENTS; i++)
	{
		CCharacter *Char = GameServer()->GetPlayerChar(i);
		if (not Char) continue;

		Char->SetHookOthers(false);
		Char->SetHitOthers(false);
		Char->SetCollideOthers(false);
		
		Controller()->teleportPlayer(i, 8);
	}

	m_turn = 0;
	m_startTick = Server()->Tick();
	setAndSayColor();
	Controller()->setPlayerTimers(g_Config.m_WwSndMgTileColors_Offset, g_Config.m_WwSndMgTileColors_Length);
}

void MGTileColors::End()
{
	for (int i=0; i<MAX_CLIENTS; i++)
	{
		CCharacter *Char = GameServer()->GetPlayerChar(i);
		if (not Char) continue;

		Controller()->teleportPlayerToSpawn(i);
	}
}

void MGTileColors::Tick()
{
	int tickCheck = (Server()->Tick() - m_startTick);
	if (tickCheck and tickCheck % 150 == 0) // do action!
	{
		for (int i=0; i<MAX_CLIENTS; i++)
		{
			CCharacter *Char = GameServer()->GetPlayerChar(i);
			if (not Char) continue;

			int Index = GameServer()->Collision()->GetMapIndex(Char->m_Pos);
			int TileIndex = GameServer()->Collision()->GetTileIndex(Index);
			if (m_currColor+185 != TileIndex) // first color begins at entity 185
			{
				// they're not inside this color, die
				float timeLeft = Controller()->getTimeLength() - Controller()->getTimer();
				Char->Die(i, WEAPON_WORLD, timeLeft/1000.f);
			}
			else if (m_turn) // they win
			{
				Controller()->winMicroGame(i);
			}
		}

		if (not m_turn)
		{
			m_turn++;
			setAndSayColor();
		}
	}
}

void MGTileColors::setAndSayColor()
{
	do m_currColor = rand() % 4;
	while (m_currColor == m_lastColor);

	m_lastColor = m_currColor;
	m_colorName = rand() % 5;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%d。\n请去%s的房间", m_turn+1, colorNames[m_currColor][m_colorName]);
	GameServer()->SendBroadcast(aBuf, -1);
}