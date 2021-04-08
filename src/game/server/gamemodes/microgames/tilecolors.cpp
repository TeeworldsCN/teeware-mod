// WarioWare mod by Headshotnoby

#include <engine/shared/config.h>
#include "tilecolors.h"

const char *colorNames[4][8] = {
	{"红色", "玫瑰色", "榴弹色", "小丑色", "番茄酱色", "马里奥色", "夏日解熟西瓜汁色", "辽阔大海沙滩上的寄居蟹色"},
	{"绿色", "草地色", "竹子色", "青蛙色", "西兰花色", "路易基色", "夏日解熟西瓜皮色", "大沙漠中长出来的仙人掌色"},
	{"蓝色", "泳池色", "天空色", "蓝莓色", "多瑙河色", "索尼克色", "Tee世界的激光色", "三精牌葡萄糖酸锌口服液色"},
	{"黄色", "柚子色", "小鸭色", "香蕉色", "硫铁矿色", "皮卡丘色", "Tee世界的太阳色", "被泥土染黄了的清澈小溪色"}
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
	m_colorName = rand() % 8;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%d.\n去%s的房间", m_turn+1, colorNames[m_currColor][m_colorName]);
	GameServer()->SendBroadcast(aBuf, -1);
}