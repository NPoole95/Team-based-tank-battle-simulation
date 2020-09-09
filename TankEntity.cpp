/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

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
	// Will be needed to implement the required tank behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Tank Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity::CTankEntity
	(
		std::vector<CVector3> patrolList,
		CTankTemplate* tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		TFloat32 maxSpeed,
		TFloat32 acceleration,
		TFloat32 turnSpeed,
		TFloat32 turretTurnSpeed,
		TFloat32 maxHP,
		TFloat32 shellDamage,
		const string& name,// = "",
		const CVector3& position,// = CVector3::kOrigin,
		const CVector3& rotation,// = CVector3(0.0f, 0.0f, 0.0f),
		const CVector3& scale// = CVector3(1.0f, 1.0f, 1.0f)

	) : CEntity(tankTemplate, UID, name, position, rotation, scale)
	{
		m_TankTemplate = tankTemplate;
		m_Timer = 0.0f;

		m_PatrolList = patrolList;
		m_State = Patrol;
		m_CurrentPatrolPoint = 0;

		m_RandomTargetPos = { 0.0f, 0.0f, 0.0f };
		m_RandomPosAssigned = false;

		// Tanks are on teams so they know who the enemy is
		m_Team = team;

		m_ShellDamage = shellDamage;
		// Initialise other tank data and state
		m_Speed = 0;
		m_MaxSpeed = maxSpeed;
		m_Acceleration = acceleration;
		m_AimSpeed = turretTurnSpeed;
		m_RotateSpeed = turnSpeed;
		m_HP = maxHP;
		m_Ammo = 10;
		m_ShellsFired = 0;

		m_ChaseCam = new CCamera({ Matrix().Position().x, 5.0f,Matrix().Position().z - 10.0f });
		m_ChaseCam->SetNearFarClip(1.0f, 2000.0f);

	}

	const float AngleMarginOfError = 1.5f; // The value used as a margin of area for rotations, if within this margin no further rotation adjustments required
	// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
	// is to be rewritten as one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CTankEntity::Update(TFloat32 updateTime)
	{
		/////////////////// variable calculation by frame time
		m_FinalAimSpeed = m_AimSpeed * updateTime;
		m_FinalRotateSpeed = m_RotateSpeed * updateTime;
		CVector3 oldChaseRotation;
		Matrix().DecomposeAffineEuler(NULL, &oldChaseRotation, NULL);

		// Fetch any messages
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			
			// Set state variables based on received messages
			switch (msg.type)
			{
				
			case Msg_Stop:
				m_State = Inactive;
				break;
			case Msg_Start:
				m_Timer = 1.0f;
				m_State = Patrol;
				break;
			case Msg_Hit:
				m_HP -= msg.damage; 
				break;
			case Msg_Evade:
				m_State = Evade;
				break;
			case Msg_AmmoDrop:
				m_AmmoCratePos = EntityManager.GetEntity("AmmoCrate")->Position();
				checkTankDirection(m_AmmoCratePos);
				m_State = ChaseAmmo;
				break;
			case Msg_OtherClaimedAmmo:
				m_RandomTargetPos = { gen::Random(Matrix().GetPosition().x - 40.0f, Matrix().GetPosition().x + 40.0f), 0.0f, gen::Random(Matrix().GetPosition().z - 40.0f, Matrix().GetPosition().z + 40.0f) };
				m_State = Evade;
				break;
			case Msg_YouClaimedAmmo:
				m_RandomTargetPos = { gen::Random(Matrix().GetPosition().x - 40.0f, Matrix().GetPosition().x + 40.0f), 0.0f, gen::Random(Matrix().GetPosition().z - 40.0f, Matrix().GetPosition().z + 40.0f) };
				checkTankDirection(m_RandomTargetPos);
				m_State = Evade;
				m_Ammo += 10;
				break;

			}
		}

		if (m_HP <= 0)
		{
			m_ChaseCam = NULL;
			return false;
		}
		if (m_State == Aim && EntityManager.GetEntity(m_Target) == nullptr) // check to see that something being aimed at has not already been destroyed, solving read access violations
		{
			m_State = Patrol;
		}

		// Tank behaviour
		if (m_State == Inactive)
		{
			m_Speed = 0;
		}
		else if (m_State == Patrol)
		{
			if (m_Ammo <= 0)
			{
				m_RandomTargetPos = { gen::Random(Matrix().GetPosition().x - 40.0f, Matrix().GetPosition().x + 40.0f), 0.0f, gen::Random(Matrix().GetPosition().z - 40.0f, Matrix().GetPosition().z + 40.0f) };
				checkTankDirection(m_RandomTargetPos);
				m_State = Evade;
			}

			Matrix(2).RotateLocalY(m_FinalAimSpeed); // slowly rotate the turret
			checkTankDirection(m_PatrolList[m_CurrentPatrolPoint]);

			////////////////// if patrol point is reached, rotate to look at new patrol point/////////////////////////////
			if (positionReached(Matrix().GetPosition(), m_PatrolList[m_CurrentPatrolPoint]))
			{
				m_Speed = 0;

				m_OldAngle = checkTankDirection(m_PatrolList[(m_CurrentPatrolPoint + 1) % m_PatrolList.size()]);

				Matrix().RotateLocalY(m_FinalRotateSpeed); // slowly rotate the tank

				// recalculate angle after adjustment
				m_NewAngle = checkTankDirection(m_PatrolList[(m_CurrentPatrolPoint + 1) % m_PatrolList.size()]);

				if (m_NewAngle > m_OldAngle)
				{
					m_RotateSpeed = -m_RotateSpeed;
				}

				if (m_NewAngle < AngleMarginOfError)
				{
					m_CurrentPatrolPoint = (m_CurrentPatrolPoint + 1) % m_PatrolList.size(); // advances to the next patrol point (loops)
				}
			}
			//////////////////////////if not, rotate to look at current patrol/////////////////////////////////
			else if (m_TankToEnemyAngle > AngleMarginOfError)
			{
				m_OldAngle = checkTankDirection(m_PatrolList[m_CurrentPatrolPoint]);

				Matrix().RotateLocalY(m_FinalRotateSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkTankDirection(m_PatrolList[m_CurrentPatrolPoint]);

				if (m_NewAngle > m_OldAngle)
				{
					m_RotateSpeed = -m_RotateSpeed;
				}

				m_Speed = 0.0f;
			}
			else
			{
				if (m_Speed < m_MaxSpeed)
				{
					m_Speed += (m_Acceleration * updateTime);
				}
				else
				{
					m_Speed = m_MaxSpeed;
				}
				Matrix().FaceTarget(m_PatrolList[m_CurrentPatrolPoint]); // face the current node in the partol
			}

			//////////////////// if an enemy is in sight, aim at them/////////////////////////////////////////
			m_Enemies = EntityManager.GetEnemiesList(GetTeam());

			// get the building entity for use in the ray cast
			EntityManager.BeginEnumEntities("", "", "Building");
			CEntity* Building = EntityManager.GetEntity("Building");



			for (int i = 0; i < m_Enemies.size(); i++)
			{
				m_Target = m_Enemies[i];
				m_FVToEnemyAngle = checkAngle(EntityManager.GetEntity(m_Target)->Position());
				CVector3 buildingSize = { 12.5f, 11.5f, 10.0f };


				// check to see whether this angle is within 15 degrees
				if (m_FVToEnemyAngle < 15 && !raycast(Building->GetUID(), buildingSize))
				{
					m_AimSpeed = 2.0f;
					m_Timer = 1.0f;
					m_State = Aim;
				}

			}

			///////////////////////////////////////////////


		}
		else if (m_State == Aim)
		{		
			// check to see if the tank is ready to fire
			if (m_Timer <= 0.0f && m_FVToEnemyAngle < AngleMarginOfError)
			{
				// FIRE
				m_TurretWorldMatrix = Matrix(2) * Matrix();
				CVector3 Rotation;
				m_TurretWorldMatrix.DecomposeAffineEuler(NULL, &Rotation, NULL);

				Rotation.x = 0.0f;
				Rotation.z = 0.0f;

				TEntityUID Shell = EntityManager.CreateShell("Shell Type 1",m_ShellDamage, GetName(), m_TurretWorldMatrix.Position(), Rotation);

				// select random position within 40 ft
				m_RandomTargetPos = { gen::Random(Matrix().GetPosition().x - 40.0f, Matrix().GetPosition().x + 40.0f), 0.0f, gen::Random(Matrix().GetPosition().z - 40.0f, Matrix().GetPosition().z + 40.0f) };
				checkTankDirection(m_RandomTargetPos);
				m_State = Evade;
				++m_ShellsFired;
				m_Ammo -= 1;
			}			
			else
			{	
				m_Speed = 0.0f;
				m_Timer -= updateTime;
			}
			//conduct the precise aiming. If the turret is looking (almost) at the target, use the look at function to stop the turret shaking
			if(m_FVToEnemyAngle > AngleMarginOfError)
			{
				m_OldAngle = checkAngle(EntityManager.GetEntity(m_Target)->Position());

				Matrix(2).RotateLocalY(m_FinalAimSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkAngle(EntityManager.GetEntity(m_Target)->Position());

				if (m_NewAngle > m_OldAngle)
				{
					m_AimSpeed = -m_AimSpeed;
				}
			}
		}
		else if (m_State == ChaseAmmo)
		{
			if (m_TankToEnemyAngle < AngleMarginOfError)
			{
				Matrix().FaceTarget(m_AmmoCratePos); // face the random position
				// move towards the position 
				if (m_Speed < m_MaxSpeed)
				{
					m_Speed += (m_Acceleration * updateTime);
				}
				else
				{
					m_Speed = m_MaxSpeed;
				}

				// if position is reached
				if (positionReached(Matrix().GetPosition(), m_AmmoCratePos))
				{
					m_Timer = 1.0f;
					m_State = Patrol;
				}
			}
			else
			{
				m_OldAngle = checkTankDirection(m_AmmoCratePos);

				Matrix().RotateLocalY(m_FinalRotateSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkTankDirection(m_AmmoCratePos);

				if (m_NewAngle > m_OldAngle)
				{
					m_RotateSpeed = -m_RotateSpeed;
				}

				//////////////////////////
				m_Speed = 0.0f;
			}

			////////////rotate turret to face forwards////////////////// 
			m_OldAngle = checkAngle(m_RandomTargetPos);

			if (m_OldAngle > AngleMarginOfError)
			{
				Matrix(2).RotateLocalY(m_FinalAimSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkAngle(m_RandomTargetPos);

				if (m_NewAngle > m_OldAngle)
				{
					m_AimSpeed = -m_AimSpeed;
				}
			}

		}
		else // evade
		{
			if (m_TankToEnemyAngle < AngleMarginOfError)
			{
				Matrix().FaceTarget(m_RandomTargetPos); // face the random position

				// move towards the position 
				if (m_Speed < m_MaxSpeed)
				{
					m_Speed += (m_Acceleration * updateTime);
				}
				else
				{
					m_Speed = m_MaxSpeed;
				}

				// if position is reached
				if (positionReached(Matrix().GetPosition(), m_RandomTargetPos))
				{
					m_Timer = 1.0f;
					m_State = Patrol;
				}
			}
			else
			{
				m_OldAngle = checkTankDirection(m_RandomTargetPos);

				Matrix().RotateLocalY(m_FinalRotateSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkTankDirection(m_RandomTargetPos);

				if (m_NewAngle > m_OldAngle)
				{
					m_RotateSpeed = -m_RotateSpeed;
				}

				//////////////////////////
				m_Speed = 0.0f;
			}
			////////////rotate turret to face forwards////////////////// 
			m_OldAngle = checkAngle(m_RandomTargetPos);
			
			if (m_OldAngle > AngleMarginOfError)
			{
				Matrix(2).RotateLocalY(m_FinalAimSpeed); // slowly rotate the turret

				// recalculate angle after adjustment
				m_NewAngle = checkAngle(m_RandomTargetPos);

				if (m_NewAngle > m_OldAngle)
				{
					m_AimSpeed = -m_AimSpeed;
				}
			}
		}

		// Perform movement...
		// Move along local Z axis scaled by update time
		Matrix().MoveLocalZ(m_Speed * updateTime);

		// code to control chase cams
		CVector3 newChaseRotation;
		Matrix().DecomposeAffineEuler(NULL, &newChaseRotation, NULL);

		m_ChaseCam->Matrix().e30 = Matrix().Position().x;
		m_ChaseCam->Matrix().e31 = 3.0;
		m_ChaseCam->Matrix().e32 = Matrix().Position().z;

		m_ChaseCam->Matrix().RotateY(newChaseRotation.y - oldChaseRotation.y);
		m_ChaseCam->Matrix().MoveLocalZ(-10.0f);

		return true; // Don't destroy the entity
	}
	bool CTankEntity::positionReached(CVector3 pos1, CVector3 pos2)
	{
		if (pos1.x >= (pos2.x - 0.2f) && pos1.x <= (pos2.x + 0.2f) &&
			pos1.z >= (pos2.z - 0.2f) && pos1.z <= (pos2.z + 0.2f))
		{
			return true;
		}
		return false;
	}

	TFloat32 CTankEntity::checkAngle(CVector3 targetPosition)
	{
		// get the matrix for the turret and use it to calculate the turrets facing vector
		m_TurretWorldMatrix = Matrix(2) * Matrix();
		m_FacingVector = { m_TurretWorldMatrix.e20, 0.0f, m_TurretWorldMatrix.e22 };

		// get the length of the facing vector
		m_FacingVectorLength = (m_FacingVector.x * m_FacingVector.x) + (m_FacingVector.z * m_FacingVector.z);
		m_FacingVectorLength = Sqrt(m_FacingVectorLength);

		// get the vector between the tank and the enemy
		m_EnemyPosition = targetPosition;
		m_VecToEnemy = { m_EnemyPosition.x - Matrix().Position().x, 0.0f, m_EnemyPosition.z - Matrix().Position().z };

		// get the length of the vector between the tank and the enemy
		m_VecToEnemyLength = (m_VecToEnemy.x * m_VecToEnemy.x) + (m_VecToEnemy.z * m_VecToEnemy.z);
		m_VecToEnemyLength = Sqrt(m_VecToEnemyLength);

		// get cos theta of the angle
		m_CosTheta = Dot(m_FacingVector, m_VecToEnemy) / (m_VecToEnemyLength * m_FacingVectorLength);

		// convert this into the final angle
		m_FVToEnemyAngle = ACos(m_CosTheta) * (180 / 3.14);

		return m_FVToEnemyAngle;
	}

	TFloat32 CTankEntity::checkTankDirection(CVector3 targetPosition)
	{
		// get the matrix for the turret and use it to calculate the turrets facing vector
		m_TurretWorldMatrix = Matrix();
		m_FacingVector = { m_TurretWorldMatrix.e20, 0.0f, m_TurretWorldMatrix.e22 };

		// get the length of the facing vector
		m_FacingVectorLength = (m_FacingVector.x * m_FacingVector.x) + (m_FacingVector.z * m_FacingVector.z);
		m_FacingVectorLength = Sqrt(m_FacingVectorLength);

		// get the vector between the tank and the enemy
		m_EnemyPosition = targetPosition;
		m_VecToEnemy = { m_EnemyPosition.x - Matrix().Position().x, 0.0f, m_EnemyPosition.z - Matrix().Position().z };

		// get the length of the vector between the tank and the enemy
		m_VecToEnemyLength = (m_VecToEnemy.x * m_VecToEnemy.x) + (m_VecToEnemy.z * m_VecToEnemy.z);
		m_VecToEnemyLength = Sqrt(m_VecToEnemyLength);

		// get cos theta of the angle
		m_CosTheta = Dot(m_FacingVector, m_VecToEnemy) / (m_VecToEnemyLength * m_FacingVectorLength);

		// convert this into the final angle
		m_TankToEnemyAngle = ACos(m_CosTheta) * (180 / 3.14);

		return m_TankToEnemyAngle;
	}

	bool CTankEntity::raycast(TEntityUID target, CVector3 targetSize)
	{

		//m_FacingVector = { m_TurretWorldMatrix.e20, 0.0f, m_TurretWorldMatrix.e22 };
		CVector3 position = EntityManager.GetEntity(target)->Position();

		m_FacingVector = {position.x - Position().x, 0, position.z - Position().z };


		float rayRange = 300.0f;  // the distance that the ray will travel, longer range means more calculations
		bool hitObject = false;      // bool to see if an object was hit
		float distToObject = 1.0f;	 // the value incremented for the ray trace (how far the ray travels before being checked) 

		float testPointX;
		float testPointY;
		float testPointZ;

		CVector3 rayTrace;

		rayTrace.x = m_FacingVector.x * distToObject;
		rayTrace.y = m_FacingVector.y * distToObject;
		rayTrace.z = (m_FacingVector.z * distToObject);

		testPointX = Matrix().Position().x + rayTrace.x;
		testPointY = Matrix().Position().y + rayTrace.y;
		testPointZ = Matrix().Position().z + rayTrace.z;



		for (distToObject = 0; distToObject < rayRange; distToObject++)
		{
			//distToObject += 1.0f;
			testPointX += rayTrace.x;
			testPointY += rayTrace.y;
			testPointZ += rayTrace.z;

			hitObject = SphereToBox(testPointX, testPointY, testPointZ, targetSize.x, targetSize.y, targetSize.z, position.x, position.y, position.z, 0.05f);
			if (hitObject)
			{
				return true;
			}


		}

		testPointX = Matrix().Position().x + rayTrace.x;
		testPointY = Matrix().Position().y + rayTrace.y;
		testPointZ = Matrix().Position().z + rayTrace.z;
		////////////////////////////////////////


		return false;
	}

} // namespace gen
