#pragma once
///////////////////////////////////////////////////////////
//  CParseLevel.cpp
//  A class to parse and setup a level (entity templates
//  and instances) from an XML file
//  Created on:      30-Jul-2005 14:40:00
//  Original author: LN
///////////////////////////////////////////////////////////

#ifndef GEN_C_PARSE_LEVEL_H_INCLUDED
#define GEN_C_PARSE_LEVEL_H_INCLUDED

#include <string>
#include <vector>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "EntityManager.h"
#include "CParseXML.h"

namespace gen
{

	/*---------------------------------------------------------------------------------------------
		CParseLevel class
	---------------------------------------------------------------------------------------------*/
	// A XML parser to read and setup a level - made up of entity templates and entity instances
	// Derived from the general CParseXML class, which performs the basic syntax parsing. The base
	// class calls functions (overridden) in this class when it encounters the start and end of
	// elements in the XML (opening and closing tags). These functions then perform appropriate
	// setup. This is an event driven system, requiring this class to store state - the entity /
	// template / member variables it is currently building
	class CParseLevel : public CParseXML
	{

		/*---------------------------------------------------------------------------------------------
			Constructors / Destructors
		---------------------------------------------------------------------------------------------*/
	public:
		// Constructor gets a pointer to the entity manager and initialises state variables
		CParseLevel(CEntityManager* entityManager);


		/*-----------------------------------------------------------------------------------------
			Private interface
		-----------------------------------------------------------------------------------------*/
	private:

		/*---------------------------------------------------------------------------------------------
			Types
		---------------------------------------------------------------------------------------------*/

		// File section currently being parsed
		enum EFileSection
		{
			None,
			Templates,
			Entities,
		};


		/*---------------------------------------------------------------------------------------------
			Callback functions
		---------------------------------------------------------------------------------------------*/

		// Callback function called when the parser meets the start of a new element (the opening tag).
		// The element name is passed as a string. The attributes are passed as a list of (C-style)
		// string pairs: attribute name, attribute value. The last attribute is marked with a null name
		void StartElt(const string& eltName, SAttribute* attrs);

		// Callback function called when the parser meets the end of an element (the closing tag). The
		// element name is passed as a string
		void EndElt(const string& eltName);


		/*---------------------------------------------------------------------------------------------
			Section Parsing
		---------------------------------------------------------------------------------------------*/

		// Called when the parser meets the start of an element (opening tag) in the templates section
		void TemplatesStartElt(const string& eltName, SAttribute* attrs);

		// Called when the parser meets the end of an element (closing tag) in the templates section
		void TemplatesEndElt(const string& eltName);

		// Called when the parser meets the start of an element (opening tag) in the entities section
		void EntitiesStartElt(const string& eltName, SAttribute* attrs);

		// Called when the parser meets the end of an element (closing tag) in the entities section
		void EntitiesEndElt(const string& eltName);

		// Called when the parser meets the start of an element (opening tag) of a entity component
		void ComponentsStartElt(const string& typeName, SAttribute* attrs);


		/*---------------------------------------------------------------------------------------------
			Data
		---------------------------------------------------------------------------------------------*/

		// Constructer is passed a pointer to an entity manager used to create templates and
		// entities as they are parsed
		CEntityManager* m_EntityManager;

		// File state
		EFileSection m_CurrentSection;

		// Current template state (i.e. latest values read during parsing)
		string   m_TemplateType;
		string   m_TemplateName;
		string   m_TemplateMesh;

		// Current entity state (i.e. latest values read during parsing)
		CEntity* m_Entity;
		string   m_EntityType;
		string   m_EntityName;
		CVector3 m_Pos;
		CVector3 m_Rot;
		CVector3 m_Scale;

		TInt32 m_Team;
		TFloat32 m_MaxSpeed;        // Maximum speed for this kind of tank
		TFloat32 m_Acceleration;    // Acceleration  -"-
		TFloat32 m_TurnSpeed;       // Turn speed    -"-
		TFloat32 m_TurretTurnSpeed; // Turret turn speed    -"-

		TUInt32  m_MaxHP;           // Maximum (initial) HP for this kind of tank
		TUInt32  m_ShellDamage;     // HP damage caused by shells from this kind of tank
		std::vector<CVector3> m_PatrolList;
	};


} // namespace gen

#endif // GEN_C_PARSE_LEVEL_H_INCLUDED
