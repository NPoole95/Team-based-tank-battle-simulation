#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{
	class CAmmoCrateEntity : public CEntity
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		CAmmoCrateEntity
		(
			CEntityTemplate* entityTemplate,
			TEntityUID       UID,
			const string& name = "",
			const CVector3& position = CVector3::kOrigin,
			const CVector3& rotation = CVector3(0.0f, 0.0f, 0.0f),
			const CVector3& scale = CVector3(1.0f, 1.0f, 1.0f)
		);

		// No destructor needed


	/////////////////////////////////////
	//	Public interface
	public:

		/////////////////////////////////////
		// Update

		// Update the Ammo Crate - performs simple shell behaviour
		// Return false if the entity is to be destroyed
		// Keep as a virtual function in case of further derivation
		virtual bool Update(TFloat32 updateTime);


		/////////////////////////////////////
		//	Private interface
	private:

		/////////////////////////////////////
		// Data

		// Add your shell data here
		bool m_AmmoCollected;
		TFloat32 m_FallSpeed; // the speed the crate will fall at if ion the air
		TFloat32 m_MoveSpeed; // the speed the crate is currently moving at
		bool m_AlertSent; // the message to tell tanks the crate has landed
		TInt32 m_AmmoContents;
		std::vector<TEntityUID> m_TanksList;
	};


} // namespace gen
#pragma once
