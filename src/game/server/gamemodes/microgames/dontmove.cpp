// WarioWare mod by Headshotnoby

#include <engine/shared/config.h>
#include "dontmove.h"

const char *modes[7][2] = {
	{"不要动，快乖乖站好！", "不要停，大家动起来！"},
	{"快站好，不要不老实！", "动起来，不要傻站着！"},
	{"快乖乖站好，不要动！", "大家动起来，不要停！"},
	{"不要不老实，快站好！", "不要傻站着，动起来！"},
	{"快站好，不要动！", "动起来，不要停！"},
	{"不要动，快站好！", "不要停，动起来！"},
	{"一二三木头人，谁动谁是小狗！", "一二三汽车人，谁不动谁爆炸！"}
};


MGDontMove::MGDontMove(CGameContext* pGameServer, CGameControllerWarioWare* pController) : Microgame(pGameServer, pController)
{
	m_microgameName = "dontmove";
	m_boss = false;
}

void MGDontMove::Start()
{
	m_Phrase = rand() % 2;
	m_Mode = rand() % 2;			
	GameServer()->SendBroadcast(modes[m_Phrase][m_Mode], -1);
	Controller()->setPlayerTimers(g_Config.m_WwSndMgDontMove_Offset, g_Config.m_WwSndMgDontMove_Length);
	for (int i=0; i<MAX_CLIENTS; i++)
	{
		CCharacter *Char = GameServer()->GetPlayerChar(i);
		if (not Char) continue;

		Controller()->g_Complete[i] = true;
		Char->SetCollideOthers(false);
		Char->SetHookOthers(false);
	}
}

void MGDontMove::End()
{
	// nothing to clean
}

void MGDontMove::Tick()
{
	float timeLeft = Controller()->getTimeLength() - Controller()->getTimer();

	for (int i=0; i<MAX_CLIENTS; i++)
	{
		CCharacter *Char = GameServer()->GetPlayerChar(i);
		if (not Char) continue;

		if (timeLeft < 2750 and timeLeft > 200)
		{
			if ((not m_Mode and (Char->IsMoving() or Char->GetInput()->m_Hook&1 or Char->GetInput()->m_Fire&1 or Char->GetInput()->m_Jump==1)) or // don't move
				(m_Mode and (not Char->IsMoving() and (Char->Core()->m_Vel.x < 1 and Char->Core()->m_Vel.x > -1) and (Char->Core()->m_Vel.y < 1 and Char->Core()->m_Vel.y > -1)))) // don't stop moving
			{
				Controller()->killAndLoseMicroGame(i);
			}
		}
	}
}