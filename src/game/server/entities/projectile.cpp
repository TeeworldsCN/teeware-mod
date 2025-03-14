/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/WarioWare.h>
#include "projectile.h"

#include <engine/shared/config.h>
#include <game/server/teams.h>

CProjectile::CProjectile
	(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeze,
		bool Explosive,
		float Force,
		int SoundImpact,
		int Weapon,
		int Layer,
		int Number
	)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	//m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = m_CreationTick = Server()->Tick();
	m_Explosive = Explosive;

	m_Layer = Layer;
	m_Number = Number;
	m_Freeze = Freeze;

	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));

	m_FootMode = 0;
	if((Dir.x < 0?-Dir.x:Dir.x) > (Dir.y < 0?-Dir.y:Dir.y))
		m_FootPickupDistance = abs(Dir.x * (float)Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeSpeed / 4000.f);
	else
		m_FootPickupDistance = abs(Dir.y * (float)Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeSpeed / 4000.f);
	m_FootBounceLoss = 50;
	m_FillExtraInfo = true;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	if(m_LifeSpan > -2)
		GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			if (!m_TuneZone)
			{
				Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
				Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			}
			else
			{
				Curvature = GameServer()->TuningList()[m_TuneZone].m_GrenadeCurvature;
				Speed = GameServer()->TuningList()[m_TuneZone].m_GrenadeSpeed;
			}

			break;

		case WEAPON_SHOTGUN:
			if (!m_TuneZone)
			{
				Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
				Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			}
			else
			{
				Curvature = GameServer()->TuningList()[m_TuneZone].m_ShotgunCurvature;
				Speed = GameServer()->TuningList()[m_TuneZone].m_ShotgunSpeed;
			}

			break;

		case WEAPON_GUN:
			if (!m_TuneZone)
			{
				Curvature = GameServer()->Tuning()->m_GunCurvature;
				Speed = GameServer()->Tuning()->m_GunSpeed;
			}
			else
			{
				Curvature = GameServer()->TuningList()[m_TuneZone].m_GunCurvature;
				Speed = GameServer()->TuningList()[m_TuneZone].m_GunSpeed;
			}
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	CGameControllerWarioWare* controller = ((CGameControllerWarioWare*)GameServer()->m_pController);

	if (m_FootMode && m_Type == WEAPON_GRENADE) // taken from teefoot mod
	{
		float PreviousTick = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
		float CurrentTick = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
		float NextTick = (Server()->Tick()-m_StartTick+1)/(float)Server()->TickSpeed();
		float TimeAlive = (Server()->Tick()-m_CreationTick)/(float)Server()->TickSpeed();

		vec2 CurPosition = GetPos(CurrentTick);
		vec2 PrevPosition = GetPos(PreviousTick);
		vec2 CollisionPosition(0,0);
		vec2 FreePosition(0,0);

		CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
		CCharacter *TChar;

		if(m_FootMode == 2 && m_LifeSpan > -1)
			m_LifeSpan--;

		if(m_LifeSpan == -1)
		{
			if(m_Explosive)
			{
				int64_t TeamMask = -1LL;
				if (OwnerChar && OwnerChar->IsAlive())
				{
					TeamMask = OwnerChar->Teams()->TeamMask(OwnerChar->Team(), -1, m_Owner);
				}

				GameServer()->CreateExplosion(CurPosition, m_Owner, m_Weapon, false, (!OwnerChar ? -1 : OwnerChar->Team()),
				(m_Owner != -1)? TeamMask : -1LL);
				GameServer()->CreateSound(CurPosition, m_SoundImpact,
				(m_Owner != -1)? TeamMask : -1LL);
			}
			GameServer()->m_World.DestroyEntity(this);
			return;
		}

		float TimeToCollision = -1.0f;
		for(float SearchTick1 = CurrentTick; SearchTick1 <= NextTick; SearchTick1 += (NextTick-CurrentTick)/30.0f)
		{
			vec2 TempPosition = GetPos(SearchTick1);
			if(GameServer()->Collision()->IsSolid((int) TempPosition.x, (int) TempPosition.y))
					break;
			TimeToCollision = SearchTick1;
		}
		if(TimeToCollision == -1.0f)
		{
			m_FootPickupDistance = 0;
			for(float SearchTick2 = CurrentTick; SearchTick2 > CurrentTick-1.0f; SearchTick2-=0.02f)
			{
				vec2 SearchPosition = GetPos(SearchTick2);
				if(!GameServer()->Collision()->IsSolid((int)SearchPosition.x, (int)SearchPosition.y))
				{
					TimeToCollision = SearchTick2;
					CollisionPosition = GetPos(SearchTick2+0.02f);
					FreePosition = GetPos(SearchTick2);
					break;
				}
			}
		}
		else
		{
			TimeToCollision += CurrentTick;
			CollisionPosition = GetPos(TimeToCollision+(NextTick-CurrentTick)/30.0f);
			FreePosition = GetPos(TimeToCollision);
		}
		if(TimeToCollision < NextTick-(NextTick-CurrentTick)/30.0f)
		{
			bool CollidedAtX = GameServer()->Collision()->IsSolid((int)CollisionPosition.x, (int)FreePosition.y);
			bool CollidedAtY = GameServer()->Collision()->IsSolid((int)FreePosition.x, (int)CollisionPosition.y);

			if(CollidedAtX)
			{
				if (m_FootMode != 2)
					m_Direction.x = -m_Direction.x/(m_FootBounceLoss+100)*100;
				else
					m_Direction.x = -m_Direction.x;

				/*
				if (m_CollisionsByX >= 50)
						GameServer()->m_World.DestroyEntity(this);
				*/
				m_CollisionsByX++;
			}
			else
			{
				if (m_FootMode != 2)
					m_Direction.x = m_Direction.x/(m_FootBounceLoss+100)*100;

				m_CollisionsByX = 0;
			}
			if(CollidedAtY)
			{
				if (m_FootMode != 2)
					m_Direction.y = -(m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*GameServer()->Tuning()->m_GrenadeSpeed*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed())/(m_FootBounceLoss+100)*100;
				else
					m_Direction.y = -(m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*GameServer()->Tuning()->m_GrenadeSpeed*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed())/(5+100)*100;

				/*
				if (m_CollisionByY >= 50)
					GameServer()->m_World.DestroyEntity(this);
				*/
				m_CollisionByY++;
			}
			else
			{
				if (m_FootMode != 2)
					m_Direction.y = (m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeSpeed)/(m_FootBounceLoss+100)*100;
				else
					m_Direction.y = (m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeSpeed)/(5+100)*100;

				m_CollisionByY = 0;
			}
			m_Pos = FreePosition;
			m_StartTick = Server()->Tick();
			m_FootPickupDistance = 0;
		}
		if(m_FootPickupDistance == 0)
			TChar = GameServer()->m_World.IntersectCharacter(PrevPosition, CurPosition, 6.0f, CurPosition, NULL);
		else
		{
			m_FootPickupDistance--;
			TChar = GameServer()->m_World.IntersectCharacter(PrevPosition, CurPosition, 6.0f, CurPosition, OwnerChar);
		}

		if(TChar) // player collided with ball
		{
			if (m_FootMode == 1 and not TChar->GetWeaponGot(WEAPON_GRENADE)) // pickup
			{
				TChar->GiveWeapon(WEAPON_GRENADE, -1);
				TChar->SetWeapon(WEAPON_GRENADE);
				TChar->SetActiveWeapon(WEAPON_GRENADE); // again just in case
				GameServer()->m_World.DestroyEntity(this);
			}
			else if (m_FootMode == 2 and TimeAlive > 1.25f) // explode (avoid spawnkill by waiting 1.25sec beforehand)
			{
				GameServer()->CreateExplosion(CurPosition, m_Owner, m_Weapon, false, (!TChar ? -1 : TChar->Team()), -1LL);
				GameServer()->CreateSound(CurPosition, m_SoundImpact, -1LL);
				GameServer()->m_World.DestroyEntity(this);
			}
		}
		return;
	}

	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	vec2 NewPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos, false);
	CCharacter *pOwnerChar = 0;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	CCharacter *pTargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar, m_Owner);

	if(m_LifeSpan > -1)
		m_LifeSpan--;

	int64_t TeamMask = -1LL;
	bool isWeaponCollide = false;
	if
	(
			pOwnerChar &&
			pTargetChr &&
			pOwnerChar->IsAlive() &&
			pTargetChr->IsAlive() &&
			!pTargetChr->CanCollide(m_Owner)
			)
	{
			isWeaponCollide = true;
			//TeamMask = OwnerChar->Teams()->TeamMask( OwnerChar->Team());
	}
	if (pOwnerChar && pOwnerChar->IsAlive())
	{
		TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);
	}
	else if (m_Owner >= 0)
	{
		GameServer()->m_World.DestroyEntity(this);
	}
	bool isMGTarget = false;
	if (controller->inMicroGame() and str_comp(controller->getMicroGame()->m_microgameName, "target") == 0)
		isMGTarget = true;

	if( ((pTargetChr && (pOwnerChar ? (isMGTarget && pTargetChr->GetPlayer()->GetCID() == MAX_CLIENTS - 1) || !(pOwnerChar->m_Hit&CCharacter::DISABLE_HIT_GRENADE) : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !isWeaponCollide)
	{
		if(m_Explosive/*??*/ && (!pTargetChr || (pTargetChr && (!m_Freeze || (m_Weapon == WEAPON_SHOTGUN && Collide)))))
		{
			if (m_Bouncing == 0)
			{
				// direct hit only
				if (isMGTarget) {
					// force damage
					if (pTargetChr)
						pTargetChr->TakeDamage(vec2(0, 0), 1, m_Owner, m_Weapon);
					// explosion
					GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, true, (!pTargetChr ? -1 : pTargetChr->Team()),
					(m_Owner != -1)? TeamMask : -1LL);
				} else {
					GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, false, (!pTargetChr ? -1 : pTargetChr->Team()),
					(m_Owner != -1)? TeamMask : -1LL);
				}

				GameServer()->CreateSound(ColPos, m_SoundImpact,
					(m_Owner != -1)? TeamMask : -1LL);
			}
		}
		else if(pTargetChr && m_Freeze && ((m_Layer == LAYER_SWITCH && GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[pTargetChr->Team()]) || m_Layer != LAYER_SWITCH))
		{
			pTargetChr->Freeze();
		}
		if(Collide && m_Bouncing != 0)
		{
			m_StartTick = Server()->Tick();
			m_Pos = NewPos+(-(m_Direction*4));
			if (m_Bouncing == 1)
				m_Direction.x = -m_Direction.x;
			else if(m_Bouncing == 2)
				m_Direction.y = -m_Direction.y;
			if (fabs(m_Direction.x) < 1e-6)
				m_Direction.x = 0;
			if (fabs(m_Direction.y) < 1e-6)
				m_Direction.y = 0;
			m_Pos += m_Direction;
		}
		else if (m_Weapon == WEAPON_GUN)
		{
			if (isMGTarget && pTargetChr)
				pTargetChr->TakeDamage(vec2(0, 0), 1, m_Owner, m_Weapon);
			GameServer()->CreateDamageInd(CurPos, -atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1)? TeamMask : -1LL);
			GameServer()->m_World.DestroyEntity(this);
		}
		else
			if (!m_Freeze)
				GameServer()->m_World.DestroyEntity(this);
	}
	if(m_LifeSpan == -1)
	{
		if(m_Explosive)
		{
			if(m_Owner >= 0)
				pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

			int64_t TeamMask = -1LL;
			if (pOwnerChar && pOwnerChar->IsAlive())
			{
					TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);
			}

			GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, false, (!pOwnerChar ? -1 : pOwnerChar->Team()),
			(m_Owner != -1)? TeamMask : -1LL);
			GameServer()->CreateSound(ColPos, m_SoundImpact,
			(m_Owner != -1)? TeamMask : -1LL);
		}
		GameServer()->m_World.DestroyEntity(this);
	}

	int x = GameServer()->Collision()->GetIndex(PrevPos, CurPos);
	int z;
	if (g_Config.m_SvOldTeleportWeapons)
		z = GameServer()->Collision()->IsTeleport(x);
	else
		z = GameServer()->Collision()->IsTeleportWeapon(x);
	if (z && ((CGameControllerWarioWare*)GameServer()->m_pController)->m_TeleOuts[z-1].size())
	{
		int Num = ((CGameControllerWarioWare*)GameServer()->m_pController)->m_TeleOuts[z-1].size();
		m_Pos = ((CGameControllerWarioWare*)GameServer()->m_pController)->m_TeleOuts[z-1][(!Num)?Num:rand() % Num];
		m_StartTick = Server()->Tick();
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CCharacter* pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%((m_Explosive)?6:20);
	if (pSnapChar && pSnapChar->IsAlive() && (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[pSnapChar->Team()] && (!Tick)))
		return;

	CCharacter *pOwnerChar = 0;
	int64_t TeamMask = -1LL;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if (pOwnerChar && pOwnerChar->IsAlive())
			TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

	if(m_Owner != -1 && !CmaskIsSet(TeamMask, SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
	{
		if(SnappingClient > -1 && not m_FootMode && m_FillExtraInfo && m_Direction != vec2(0,0) && GameServer()->m_apPlayers[SnappingClient] && GameServer()->m_apPlayers[SnappingClient]->m_ClientVersion >= VERSION_DDNET_ANTIPING_PROJECTILE)
			FillExtraInfo(pProj);
		else
			FillInfo(pProj);
	}
}

// DDRace

void CProjectile::SetBouncing(int Value)
{
	m_Bouncing = Value;
}

void CProjectile::FillExtraInfo(CNetObj_Projectile *pProj)
{
	const int MaxPos = 0x7fffffff/100;
	if(abs((int)m_Pos.y)+1 >= MaxPos || abs((int)m_Pos.x)+1 >= MaxPos)
	{
		//If the modified data would be too large to fit in an integer, send normal data instead
		FillInfo(pProj);
		return;
	}
	//Send additional/modified info, by modifiying the fields of the netobj
	float Angle = -atan2f(m_Direction.x, m_Direction.y);

	int Data = 0;
	Data |= (abs(m_Owner) & 255)<<0;
	if(m_Owner < 0)
		Data |= 1<<8;
	Data |= 1<<9; //This bit tells the client to use the extra info
	Data |= (m_Bouncing & 3)<<10;
	if(m_Explosive)
		Data |= 1<<12;
	if(m_Freeze)
		Data |= 1<<13;

	pProj->m_X = (int)(m_Pos.x * 100.0f);
	pProj->m_Y = (int)(m_Pos.y * 100.0f);
	pProj->m_VelX = (int)(Angle * 1000000.0f);
	pProj->m_VelY = Data;
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}
