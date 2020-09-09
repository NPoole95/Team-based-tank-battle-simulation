/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
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
extern TEntityUID GetTankUID( int team );




/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Shell constructor intialises shell-specific data and passes its parameters to the base
// class constructor
CShellEntity::CShellEntity
(
	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	TInt32 damage,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	// Initialise any shell data you add
	m_ShellLife = 2.5f;
	m_ShellSpeed = 100.0f;
	m_ShellRadius = 0.5f;
	m_ShellDamage = damage;
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
// one of the assignment requirements
// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{

	if (m_ShellLife <= 0.0f)
	{
		return false;
	}
	else
	{
		m_ShellLife -= updateTime;
	}

	//////////// get list of all tanks /////////////////
	m_TanksList = EntityManager.GetList("Tank");

	// cycles through every other tank and sees if the shell has hit them
	for (int i = 0; i < m_TanksList.size(); i++)
	{	
		if (EntityManager.GetEntity(m_TanksList[i])->GetName() != GetName()) // ensure you cant hit yourself
		{
			CVector3 enemyPosition = EntityManager.GetEntity(m_TanksList[i])->Position();

			if (SphereToBox(Position().x, Position().y, Position().z, 3.5f, 3.0f, 6.0f, enemyPosition.x, enemyPosition.y, enemyPosition.z, m_ShellRadius))
			{
				
				SMessage hit;
				hit.damage = m_ShellDamage;
				hit.from = GetUID();
				hit.type = Msg_Hit;
				Messenger.SendMessageA(m_TanksList[i], hit);
				return false;
			}
		}
	}
	// Perform movement...
	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ(m_ShellSpeed * updateTime);

	return true; // Placeholder
}


} // namespace gen
