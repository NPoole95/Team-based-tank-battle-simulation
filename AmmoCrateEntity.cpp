/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "AmmoCrateEntity.h"
#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

	// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
	// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
	//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
	// Will be needed to implement the required shell behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Shell Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CAmmoCrateEntity::CAmmoCrateEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		// Initialise any ammo crate data you add

		m_AmmoCollected = false;
		m_AlertSent = false;
		m_FallSpeed = 8.0f;
		m_MoveSpeed = m_FallSpeed;
		m_AmmoContents = 10;
	}


	// Update the ammo crate - controls its behaviour.
	// Return false if the entity is to be destroyed
	bool CAmmoCrateEntity::Update(TFloat32 updateTime)
	{

		if (m_AmmoCollected == true)
		{
			return false;
		}
		
		if (Position().y <= 1.0f)
		{
			Position().y = 1.0f;
			m_MoveSpeed = 0.0f;

			if (m_AlertSent == false)
			{
				SMessage CrateDropped;
				CrateDropped.type = Msg_AmmoDrop;
				for (int i = 0; i < m_TanksList.size(); i++)
				{
					Messenger.SendMessageA(m_TanksList[i], CrateDropped);
				}

				m_AlertSent = true;
			}

		}
		else
		{
			m_MoveSpeed = m_FallSpeed;
		}

		//////////// get list of all tanks /////////////////
		m_TanksList = EntityManager.GetList("Tank");

		// cycles through every other tank and sees if the shell has hit them
		for (int i = 0; i < m_TanksList.size(); i++)
		{
			CVector3 enemyPosition = EntityManager.GetEntity(m_TanksList[i])->Position();

			if (SphereToBox(Position().x, Position().y, Position().z, 3.5f, 3.0f, 6.0f, enemyPosition.x, enemyPosition.y, enemyPosition.z, 0.5f))
			{
				SMessage AmmoClaimed;
				AmmoClaimed.from = GetUID();
				for (int j = 0; j < m_TanksList.size(); j++)
				{
					if (m_TanksList[i] == m_TanksList[j])
					{
						AmmoClaimed.type = Msg_YouClaimedAmmo;
					}
					else
					{
						AmmoClaimed.type = Msg_OtherClaimedAmmo;
					}
					Messenger.SendMessageA(m_TanksList[j], AmmoClaimed);
				}
				return false;
			}
			
		}
		// Perform movement...
		// Move along local Z axis scaled by update time
		Matrix().MoveLocalY(-m_MoveSpeed * updateTime);

		return true; // Placeholder
	}


} // namespace gen
