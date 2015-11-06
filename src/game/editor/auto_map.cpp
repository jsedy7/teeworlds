#include <cassert>
#include <iostream>

#include <engine/textrender.h>
#include <engine/storage.h>

#include "editor.h"
#include "auto_map.h"



void CTilesetMapper::Load(const json_value &rElement)
{
	for(unsigned i = 0; i < rElement.u.array.length; ++i)
	{
		if(rElement[i].u.object.length != 1)
			continue;

		// new rule set
		CRuleSet NewRuleSet;
		const char* pConfName = rElement[i].u.object.values[0].name;
		str_copy(NewRuleSet.m_aName, pConfName, sizeof(NewRuleSet.m_aName));
		const json_value &rStart = *(rElement[i].u.object.values[0].value);

		// get basetile
		const json_value &rBasetile = rStart["basetile"];
		if(rBasetile.type == json_integer)
			NewRuleSet.m_BaseTile = clamp((int)rBasetile.u.integer, 0, 255);
		else
			NewRuleSet.m_BaseTile = 1;

		// get rules
		const json_value &rRuleNode = rStart["rules"];
		for(unsigned j = 0; j < rRuleNode.u.array.length && j < MAX_RULES; j++)
		{
			// create a new rule
			CRule NewRule;

			// index
			const json_value &rIndex = rRuleNode[j]["index"];
			if(rIndex.type == json_integer)
				NewRule.m_Index = clamp((int)rIndex.u.integer, 0, 255);
			else
				NewRule.m_Index = 1;

			// random
			const json_value &rRandom = rRuleNode[j]["random"];
			if(rRandom.type == json_integer)
				NewRule.m_Random = clamp((int)rRandom.u.integer, 0, 99999);
			else
				NewRule.m_Random = 0;

			// rotate
			const json_value &rRotate = rRuleNode[j]["rotate"];
			if(rRotate.type == json_integer && (rRotate.u.integer == 90 || rRotate.u.integer == 180 || rRotate.u.integer == 270))
				NewRule.m_Rotation = rRotate.u.integer;
			else
				NewRule.m_Rotation = 0;

			// hflip
			const json_value &rHFlip = rRuleNode[j]["hflip"];
			if(rHFlip.type == json_integer)
				NewRule.m_HFlip = clamp((int)rHFlip.u.integer, 0, 1);
			else
				NewRule.m_HFlip = 0;

			// vflip
			const json_value &rVFlip = rRuleNode[j]["vflip"];
			if(rVFlip.type == json_integer)
				NewRule.m_VFlip = clamp((int)rVFlip.u.integer, 0, 1);
			else
				NewRule.m_VFlip = 0;

			// get rule's content
			const json_value &rCondition = rRuleNode[j]["condition"];
			if(rCondition.type == json_array)
			{
				for(unsigned k = 0; k < rCondition.u.array.length; k++)
				{
					CRuleCondition Condition;

					Condition.m_X = rCondition[k]["x"].u.integer;
					Condition.m_Y = rCondition[k]["y"].u.integer;
					const json_value &rValue = rCondition[k]["value"];
					if(rValue.type == json_string)
					{
						// the value is not an index, check if it's full or empty
						if(str_comp((const char *)rValue, "full") == 0)
							Condition.m_Value = CRuleCondition::FULL;
						else
							Condition.m_Value = CRuleCondition::EMPTY;
					}
					else if(rValue.type == json_integer)
						Condition.m_Value = clamp((int)rValue.u.integer, (int)CRuleCondition::EMPTY, 255);
					else
						Condition.m_Value = CRuleCondition::EMPTY;

					NewRule.m_aConditions.add(Condition);
				}
			}

			NewRuleSet.m_aRules.add(NewRule);
		}

		m_aRuleSets.add(NewRuleSet);
	}
}

const char* CTilesetMapper::GetRuleSetName(int Index) const
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CTilesetMapper::Proceed(CLayerTiles *pLayer, int ConfigID)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;

	int BaseTile = pConf->m_BaseTile;

	// auto map !
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	for(int y = 0; y < pLayer->m_Height; y++)
		for(int x = 0; x < pLayer->m_Width; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			if(pTile->m_Index == 0)
				continue;

			pTile->m_Index = BaseTile;

			for(int i = 0; i < pConf->m_aRules.size(); ++i)
			{
				bool RespectRules = true;
				for(int j = 0; j < pConf->m_aRules[i].m_aConditions.size() && RespectRules; ++j)
				{
					CRuleCondition *pCondition = &pConf->m_aRules[i].m_aConditions[j];
					int CheckIndex = clamp((y+pCondition->m_Y), 0, pLayer->m_Height-1)*pLayer->m_Width+clamp((x+pCondition->m_X), 0, pLayer->m_Width-1);

					if(CheckIndex < 0 || CheckIndex >= MaxIndex)
						RespectRules = false;
					else
					{
						if(pCondition->m_Value == CRuleCondition::EMPTY || pCondition->m_Value == CRuleCondition::FULL)
						{
							if(pLayer->m_pTiles[CheckIndex].m_Index > 0 && pCondition->m_Value == CRuleCondition::EMPTY)
								RespectRules = false;

							if(pLayer->m_pTiles[CheckIndex].m_Index == 0 && pCondition->m_Value == CRuleCondition::FULL)
								RespectRules = false;
						}
						else
						{
							if(pLayer->m_pTiles[CheckIndex].m_Index != pCondition->m_Value)
								RespectRules = false;
						}
					}
				}

				if(RespectRules && (pConf->m_aRules[i].m_Random <= 1 || (int)((float)rand() / ((float)RAND_MAX + 1) * pConf->m_aRules[i].m_Random) == 1))
				{
					pTile->m_Index = pConf->m_aRules[i].m_Index;
					pTile->m_Flags = 0;

					// rotate
					if(pConf->m_aRules[i].m_Rotation == 90)
						pTile->m_Flags ^= TILEFLAG_ROTATE;
					else if(pConf->m_aRules[i].m_Rotation == 180)
						pTile->m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP);
					else if(pConf->m_aRules[i].m_Rotation == 270)
						pTile->m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP|TILEFLAG_ROTATE);

					// flip
					if(pConf->m_aRules[i].m_HFlip)
						pTile->m_Flags ^= pTile->m_Flags&TILEFLAG_ROTATE ? TILEFLAG_HFLIP : TILEFLAG_VFLIP;
					if(pConf->m_aRules[i].m_VFlip)
						pTile->m_Flags ^= pTile->m_Flags&TILEFLAG_ROTATE ? TILEFLAG_VFLIP : TILEFLAG_HFLIP;
				}
			}
		}

	m_pEditor->m_Map.m_Modified = true;
}

int CompareRules(const void *a, const void *b)
{
	CDoodadsMapper::CRule *ra = (CDoodadsMapper::CRule*)a;
	CDoodadsMapper::CRule *rb = (CDoodadsMapper::CRule*)b;

	if((ra->m_Location == CDoodadsMapper::CRule::FLOOR && rb->m_Location == CDoodadsMapper::CRule::FLOOR)
		|| (ra->m_Location == CDoodadsMapper::CRule::CEILING && rb->m_Location  == CDoodadsMapper::CRule::CEILING))
	{
		if(ra->m_Size.x < rb->m_Size.x)
			return +1;
		if(rb->m_Size.x < ra->m_Size.x)
			return -1;
	}
	else if(ra->m_Location == CDoodadsMapper::CRule::WALLS && rb->m_Location == CDoodadsMapper::CRule::WALLS)
	{
		if(ra->m_Size.y < rb->m_Size.y)
			return +1;
		if(rb->m_Size.y < ra->m_Size.y)
			return -1;
	}

	return 0;
}

void CDoodadsMapper::Load(const json_value &rElement)
{
	for(unsigned i = 0; i < rElement.u.array.length; ++i)
	{
		if(rElement[i].u.object.length != 1)
			continue;

		// new rule set
		CRuleSet NewRuleSet;
		const char* pConfName = rElement[i].u.object.values[0].name;
		str_copy(NewRuleSet.m_aName, pConfName, sizeof(NewRuleSet.m_aName));
		const json_value &rStart = *(rElement[i].u.object.values[0].value);

		// get rules
		const json_value &rRuleNode = rStart["rules"];
		for(unsigned j = 0; j < rRuleNode.u.array.length && j < MAX_RULES; j++)
		{
			// create a new rule
			CRule NewRule;

			// startid
			const json_value &rStartid = rRuleNode[j]["startid"];
			if(rStartid.type == json_integer)
				NewRule.m_Rect.x = clamp((int)rStartid.u.integer, 0, 255);
			else
				NewRule.m_Rect.x = 1;

			// endid
			const json_value &rEndid = rRuleNode[j]["endid"];
			if(rEndid.type == json_integer)
				NewRule.m_Rect.y = clamp((int)rEndid.u.integer, 0, 255);
			else
				NewRule.m_Rect.y = 1;

			// broken, skip
			if(NewRule.m_Rect.x > NewRule.m_Rect.y)
				continue;

			// rx
			const json_value &rRx = rRuleNode[j]["rx"];
			if(rRx.type == json_integer)
				NewRule.m_RelativePos.x = rRx.u.integer;
			else
				NewRule.m_RelativePos.x = 0;

			// ry
			const json_value &rRy = rRuleNode[j]["ry"];
			if(rRy.type == json_integer)
				NewRule.m_RelativePos.y = rRy.u.integer;
			else
				NewRule.m_RelativePos.y = 0;

			// width
			NewRule.m_Size.x = absolute(NewRule.m_Rect.x-NewRule.m_Rect.y)%16+1;
			// height
			NewRule.m_Size.y = floor((float)absolute(NewRule.m_Rect.x-NewRule.m_Rect.y)/16)+1;

			// random
			const json_value &rRandom = rRuleNode[j]["random"];
			if(rRandom.type == json_integer)
				NewRule.m_Random = clamp((int)rRandom.u.integer, 1, 99999);
			else
				NewRule.m_Random = 1;

			// hflip
			const json_value &rHFlip = rRuleNode[j]["hflip"];
			if(rHFlip.type == json_integer)
				NewRule.m_HFlip = clamp((int)rHFlip.u.integer, 0, 1);
			else
				NewRule.m_HFlip = 0;

			// vflip
			const json_value &rVFlip = rRuleNode[j]["vflip"];
			if(rVFlip.type == json_integer)
				NewRule.m_VFlip = clamp((int)rVFlip.u.integer, 0, 1);
			else
				NewRule.m_VFlip = 0;

			// location
			NewRule.m_Location = CRule::FLOOR;
			const json_value &rLocation = rRuleNode[j]["location"];
			if(rLocation.type == json_string)
			{
				if(str_comp((const char *)rLocation, "ceiling") == 0)
					NewRule.m_Location = CRule::CEILING;
				else if(str_comp((const char *)rLocation, "walls") == 0)
					NewRule.m_Location = CRule::WALLS;
			}

			NewRuleSet.m_aRules.add(NewRule);
		}

		m_aRuleSets.add(NewRuleSet);
	}

	// sort
	for(int i = 0; i < m_aRuleSets.size(); i++)
		qsort(m_aRuleSets[i].m_aRules.base_ptr(), m_aRuleSets[i].m_aRules.size(), sizeof(m_aRuleSets[i].m_aRules[0]), CompareRules);
}

const char* CDoodadsMapper::GetRuleSetName(int Index) const
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CDoodadsMapper::AnalyzeGameLayer()
{
	// the purpose of this is to detect game layer collision's edges to place doodads according to them

	// clear existing edges
	m_FloorIDs.clear();
	m_CeilingIDs.clear();
	m_RightWallIDs.clear();
	m_LeftWallIDs.clear();

	CLayerGame *pLayer = m_pEditor->m_Map.m_pGameLayer;

	bool FloorKeepChaining = false;
	bool CeilingKeepChaining = false;
	int FloorChainID = 0;
	int CeilingChainID = 0;

	// floors and ceilings
	// browse up to down
	for(int y = 1; y < pLayer->m_Height-1; y++)
	{
		FloorKeepChaining = false;
		CeilingKeepChaining = false;

		for(int x = 1; x < pLayer->m_Width-1; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);

			// empty, skip
			if(pTile->m_Index == 0)
			{
				FloorKeepChaining = false;
				CeilingKeepChaining = false;
				continue;
			}

			// check up
			int CheckIndex = (y-1)*pLayer->m_Width+x;

			// add a floor part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!FloorKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					FloorChainID = m_FloorIDs.add(aChain);
					FloorKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_FloorIDs[FloorChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				FloorKeepChaining = false;

			// check down
			CheckIndex = (y+1)*pLayer->m_Width+x;

			// add a ceiling part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!CeilingKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					CeilingChainID = m_CeilingIDs.add(aChain);
					CeilingKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_CeilingIDs[CeilingChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				CeilingKeepChaining = false;
		}
	}

	bool RWallKeepChaining = false;
	bool LWallKeepChaining = false;
	int RWallChainID = 0;
	int LWallChainID = 0;

	// walls
	// browse left to right
	for(int x = 1; x < pLayer->m_Width-1; x++)
	{
		RWallKeepChaining = false;
		LWallKeepChaining = false;

		for(int y = 1; y < pLayer->m_Height-1; y++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);

			if(pTile->m_Index == 0)
			{
				RWallKeepChaining = false;
				LWallKeepChaining = false;
				continue;
			}

			// check right
			int CheckIndex = y*pLayer->m_Width+(x+1);

			// add a right wall part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!RWallKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					RWallChainID = m_RightWallIDs.add(aChain);
					RWallKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_RightWallIDs[RWallChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				RWallKeepChaining = false;

			// check left
			CheckIndex = y*pLayer->m_Width+(x-1);

			// add a left wall part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!LWallKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					LWallChainID = m_LeftWallIDs.add(aChain);
					LWallKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_LeftWallIDs[LWallChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				LWallKeepChaining = false;
		}
	}

	// clean up too small chains
	for(int i = 0; i < m_FloorIDs.size(); i++)
	{
		if(m_FloorIDs[i].size() < 3)
		{
			m_FloorIDs.remove_index_fast(i);
			i--;
		}
	}

	for(int i = 0; i < m_CeilingIDs.size(); i++)
	{
		if(m_CeilingIDs[i].size() < 3)
		{
			m_CeilingIDs.remove_index_fast(i);
			i--;
		}
	}

	for(int i = 0; i < m_RightWallIDs.size(); i++)
	{
		if(m_RightWallIDs[i].size() < 3)
		{
			m_RightWallIDs.remove_index_fast(i);
			i--;
		}
	}

	for(int i = 0; i < m_LeftWallIDs.size(); i++)
	{
		if(m_LeftWallIDs[i].size() < 3)
		{
			m_LeftWallIDs.remove_index_fast(i);
			i--;
		}
	}
}

void CDoodadsMapper::PlaceDoodads(CLayerTiles *pLayer, CRule *pRule, array<array<int> > *pPositions, int Amount, int LeftWall)
{
	if(pRule->m_Location == CRule::CEILING)
		pRule->m_RelativePos.y++;
	else if(pRule->m_Location == CRule::WALLS)
		pRule->m_RelativePos.x++;

	// left walls
	if(LeftWall)
	{
		pRule->m_HFlip ^= LeftWall;
		pRule->m_RelativePos.x--;
		pRule->m_RelativePos.x = -pRule->m_RelativePos.x;
		pRule->m_RelativePos.x -= pRule->m_Size.x-1;
	}

	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	int RandomValue = pRule->m_Random*((101.f-Amount)/100.0f);

	if(pRule->m_Random == 1)
		RandomValue = 1;

	// allow diversity with high Amount
	if(pRule->m_Random > 1 && RandomValue <= 1)
		RandomValue = 2;

	for(int f = 0; f < pPositions->size(); f++)
		for(int c = 0; c < (*pPositions)[f].size(); c += pRule->m_Size.x)
		{
			if((pRule->m_Location == CRule::FLOOR || pRule->m_Location == CRule::CEILING)
				&& (*pPositions)[f].size()-c < pRule->m_Size.x)
			break;

			if(pRule->m_Location == CRule::WALLS && (*pPositions)[f].size()-c < pRule->m_Size.y)
				break;

			if(RandomValue > 1 && !IAutoMapper::Random(RandomValue))
				continue;

			// where to put it
			int ID = (*pPositions)[f][c];

			// relative position
			ID += pRule->m_RelativePos.x;
			ID += pRule->m_RelativePos.y*pLayer->m_Width;

			for(int y = 0; y < pRule->m_Size.y; y++)
				for(int x = 0; x < pRule->m_Size.x; x++)
				{
					int Index = y*pLayer->m_Width+x+ID;
					if(Index <= 0 || Index >= MaxIndex)
						continue;

					pLayer->m_pTiles[Index].m_Index = pRule->m_Rect.x+y*16+x;
				}

			// hflip
			if(pRule->m_HFlip)
			{
				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x/2; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;

						int CheckIndex = Index+pRule->m_Size.x-1-x*2;

						if(CheckIndex <= 0 || CheckIndex >= MaxIndex)
							continue;

						CTile Tmp = pLayer->m_pTiles[Index];
						pLayer->m_pTiles[Index] = pLayer->m_pTiles[CheckIndex];
						pLayer->m_pTiles[CheckIndex] = Tmp;
					}

				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;

						pLayer->m_pTiles[Index].m_Flags ^= TILEFLAG_VFLIP;
					}
			}

			// vflip
			if(pRule->m_VFlip)
			{
				for(int y = 0; y < pRule->m_Size.y/2; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;

						int CheckIndex = Index+(pRule->m_Size.y-1-y*2)*pLayer->m_Width;

						if(CheckIndex <= 0 || CheckIndex >= MaxIndex)
							continue;

						CTile Tmp = pLayer->m_pTiles[Index];
						pLayer->m_pTiles[Index] = pLayer->m_pTiles[CheckIndex];
						pLayer->m_pTiles[CheckIndex] = Tmp;
					}

				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;

						pLayer->m_pTiles[Index].m_Flags ^= TILEFLAG_HFLIP;
					}
			}

			// make the place occupied
			if(RandomValue > 1)
			{
				array<int> aChainBefore;
				array<int> aChainAfter;

				for(int j = 0; j < c; j++)
					aChainBefore.add((*pPositions)[f][j]);

				int Size = pRule->m_Size.x;
				if(pRule->m_Location == CRule::WALLS)
					Size = pRule->m_Size.y;

				for(int j = c+Size; j < (*pPositions)[f].size(); j++)
					aChainAfter.add((*pPositions)[f][j]);

				pPositions->remove_index(f);

				// f changes, reset c
				c = -1;

				if(aChainBefore.size() > 1)
					pPositions->add(aChainBefore);
				if(aChainAfter.size() > 1)
					pPositions->add(aChainAfter);
			}
		}
}

void CDoodadsMapper::Proceed(CLayerTiles *pLayer, int ConfigID, int Amount)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;

	AnalyzeGameLayer();

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;

	int MaxIndex = pLayer->m_Width*pLayer->m_Height;

	// clear tiles
	for(int i = 0 ; i < MaxIndex; i++)
	{
		pLayer->m_pTiles[i].m_Index = 0;
		pLayer->m_pTiles[i].m_Flags = 0;
	}

	// place doodads
	for(int i = 0 ; i < pConf->m_aRules.size(); i++)
	{
		CRule *pRule = &pConf->m_aRules[i];

		// floors
		if(pRule->m_Location == CRule::FLOOR && m_FloorIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_FloorIDs, Amount);
		}

		// ceilings
		if(pRule->m_Location == CRule::CEILING && m_CeilingIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_CeilingIDs, Amount);
		}

		// right walls
		if(pRule->m_Location == CRule::WALLS && m_RightWallIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_RightWallIDs, Amount);
		}

		// left walls
		if(pRule->m_Location == CRule::WALLS && m_LeftWallIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_LeftWallIDs, Amount, 1);
		}
	}

	m_pEditor->m_Map.m_Modified = true;
}


//
// CMapFilter
//

CMapFilter::CPattern::CPattern()
{
	// unrelevent
	m_Width = 0;
	m_Height = 0;
	m_Weight = 1.0f;
}

void CMapFilter::CPattern::LoadTiles(const json_value& JsonVal)
{
	int t = 0;
	for(unsigned i = 0; i < JsonVal.u.array.length; i++)
	{
		bool valid = false;

		const json_value& rTile = JsonVal[i];
		if(rTile.type == json_integer)
		{
			m_aTiles[t].m_Index = clamp((int)rTile.u.integer, (int)CMapFilter::FULL, 255);
			valid = true;
		}
		else if(rTile.type == json_object)
		{
			const json_value& rIndex = rTile["index"];
			if(rIndex.type == json_integer)
			{
				m_aTiles[t].m_Index = clamp((int)rIndex.u.integer, (int)CMapFilter::FULL, 255);
				valid = true;
			}

			const json_value& rFlags = rTile["flags"];
			if(rFlags.type == json_integer)
				m_aTiles[t].m_Flags = clamp((int)rFlags.u.integer, 0, 128);
		}

		if(valid)
			t++;
	}
}

json_value* CMapFilter::CPattern::TilesToJson() const
{
	json_value* pJsonTiles = json_array_new(m_aTiles.size());

	for(int i = 0; i < m_aTiles.size(); i++)
	{
		if(m_aTiles[i].m_Flags == 0)
			json_array_push(pJsonTiles, json_integer_new(m_aTiles[i].m_Index));
		else
		{
			json_value* pJsonTileObj = json_object_new(2);
			json_object_push(pJsonTileObj, "index", json_integer_new(m_aTiles[i].m_Index));
			json_object_push(pJsonTileObj, "flags", json_integer_new(m_aTiles[i].m_Flags));
			json_array_push(pJsonTiles, pJsonTileObj);
		}
	}

	return pJsonTiles;
}

void CMapFilter::CPattern::Print()
{
	std::cout << "weight: " << m_Weight << std::endl;
	std::cout << "tiles: [";
	for(int i = 0; i < m_aTiles.size(); i++)
		std::cout << m_aTiles[i].m_Index << ",";
	std::cout << "]" << std::endl;
}

void CMapFilter::CPattern::SetSize(int Width, int Height)
{
	m_Width = Width;
	m_Height = Height;
	m_aTiles.set_size(m_Width*m_Height);
}

void CMapFilter::CPattern::SetTile(int x, int y, int Index, int Flags)
{
	x = clamp(x, 0, m_Width);
	y = clamp(y, 0, m_Height);
	int i = y*m_Width+x;
	m_aTiles[i].m_Index = clamp(Index, 0, 255);
	m_aTiles[i].m_Flags = Flags;
}

const CMapFilter::CMFTile& CMapFilter::CPattern::GetTile(int TileID) const
{
	return m_aTiles[TileID];
}

const array<CMapFilter::CMFTile>& CMapFilter::CPattern::GetTiles() const
{
	return m_aTiles;
}

void CMapFilter::CPattern::Clear()
{
	for(int i = 0; i < m_aTiles.size(); i++)
		m_aTiles[i].Clear();
}

CMapFilter::CMapFilter()
{
	m_Width = 0;
	m_Height = 0;
	m_RefPointId = 0;
}

CMapFilter::CMapFilter(const json_value& JsonVal)
{
	/*
	  "width": int,
	  "height": int,
	  "filter": [index,..., { "index": index, "flags": int }, ...],
	  "patterns": [
			{
				"weight": float,
				"tiles": [index,..., { index, flags }, ...]
			},
			{
				"weight": float,
				"tiles": [index,..., { index, flags }, ...]
			},
			...
	  ]
	*/
	std::cout << "CMapFilter(const json_value& JsonVal)" << std::endl;

	m_Width = 0;
	const json_value& w = JsonVal["width"];
	if(w.type == json_integer)
		m_Width = clamp((int)w.u.integer, 0, (int)MAX_SIZE);

	m_Height = 0;
	const json_value& h = JsonVal["height"];
	if(h.type == json_integer)
		m_Height = clamp((int)h.u.integer, 0, (int)MAX_SIZE);

	// init array
	SetSize(m_Width, m_Height);
	Clear();

	const json_value& rFilter = JsonVal["filter"];
	int f = 0;
	if(rFilter.type == json_array)
		for(unsigned i = 0; i < rFilter.u.array.length; i++)
		{
			bool valid = false;

			const json_value& rTile = rFilter[i];
			if(rTile.type == json_integer)
			{
				m_aFilter[f].m_Index = clamp((int)rTile.u.integer, (int)CMapFilter::FULL, 255);
				valid = true;
			}
			else if(rTile.type == json_object)
			{
				const json_value& rIndex = rTile["index"];
				if(rIndex.type == json_integer)
				{
					m_aFilter[f].m_Index = clamp((int)rIndex.u.integer, (int)CMapFilter::FULL, 255);
					valid = true;
				}

				const json_value& rFlags = rTile["flags"];
				if(rFlags.type == json_integer)
					m_aFilter[f].m_Flags = clamp((int)rFlags.u.integer, 0, 128);
			}

			if(valid)
				f++;
		}

	// ref point
	for(int i = 0; i < m_aFilter.size(); i++)
	{
		if(m_aFilter[i].m_Index != CMapFilter::EMPTY)
		{
			m_RefPointId = i;
			break;
		}
	}

	const json_value& rPatternArr = JsonVal["patterns"];
	int p = 0;
	if(rPatternArr.type == json_array)
		for(unsigned i = 0; i < rPatternArr.u.array.length; i++)
		{
			const json_value& rPattern = rPatternArr[i];
			if(rPattern.type == json_object)
			{
				float Weight = 0.f;
				const json_value& w = rPattern["weight"];
				if(w.type == json_double)
					Weight = clamp((float)w.u.dbl, 0.f, 1.f);

				const json_value& rTiles = rPattern["tiles"];
				if(rTiles.type == json_array && (int)rTiles.u.array.length == m_aFilter.size())
				{
					AddPattern();
					m_aPatterns[p].LoadTiles(rTiles);
					SetPatternWeight(p, Weight);
					p++;
				}
			}
		}
}

json_value* CMapFilter::ToJson() const
{
	/*
	  "width": int,
	  "height": int,
	  "filter": [index,..., { "index": index, "flags": int }, ...],
	  "patterns": [
			{
				"weight": float,
				"tiles": [index,..., { index, flags }, ...]
			},
			{
				"weight": float,
				"tiles": [index,..., { index, flags }, ...]
			},
			...
	  ]
	*/

	json_value* pJsonObj = json_object_new(4);
	json_object_push(pJsonObj, "width", json_integer_new(m_Width));
	json_object_push(pJsonObj, "height", json_integer_new(m_Height));

	json_value* pJsonFilter = json_array_new(m_aFilter.size());
	for(int i = 0; i < m_aFilter.size(); i++)
	{
		if(m_aFilter[i].m_Flags == 0)
			json_array_push(pJsonFilter, json_integer_new(m_aFilter[i].m_Index));
		else
		{
			json_value* pJsonTileObj = json_object_new(2);
			json_object_push(pJsonTileObj, "index", json_integer_new(m_aFilter[i].m_Index));
			json_object_push(pJsonTileObj, "flags", json_integer_new(m_aFilter[i].m_Flags));
			json_array_push(pJsonFilter, pJsonTileObj);
		}
	}

	json_object_push(pJsonObj, "filter", pJsonFilter);

	json_value* pJsonPatts = json_array_new(m_aPatterns.size());
	for(int i = 0; i < m_aPatterns.size(); i++)
	{
		json_value* pJsonPattObj = json_object_new(2);
		json_object_push(pJsonPattObj, "weight", json_double_new(m_aPatterns[i].m_Weight));
		json_object_push(pJsonPattObj, "tiles", m_aPatterns[i].TilesToJson());
		json_array_push(pJsonPatts, pJsonPattObj);
	}

	json_object_push(pJsonObj, "patterns", pJsonPatts);

	return pJsonObj;
}

void CMapFilter::Print()
{
	std::cout << "CMapFilter [" << this << "]" << std::endl;
	std::cout << "w: " << m_Width << " h: " << m_Height << std::endl;
	std::cout << "ref: " << m_RefPointId << std::endl;
	std::cout << "filter: [";
	for(int i = 0; i < m_aFilter.size(); i++)
		std::cout << m_aFilter[i].m_Index << ",";
	std::cout << "]" << std::endl;
	std::cout << "patterns: [" << std::endl;;
	for(int i = 0; i < m_aPatterns.size(); i++)
	{
		std::cout << i << std::endl;
		m_aPatterns[i].Print();
	}
	std::cout << "]" << std::endl;
}

void CMapFilter::SetSize(int Width, int Height)
{
	m_Width = clamp(Width, 0, (int)CMapFilter::MAX_SIZE);
	m_Height = clamp(Height, 0, (int)CMapFilter::MAX_SIZE);
	m_aFilter.set_size(m_Width*m_Height);

	for(int i = 0; i < m_aPatterns.size(); i++)
		m_aPatterns[i].SetSize(m_Width, m_Height);
}

int CMapFilter::GetWidth() const
{
	return m_Width;
}

void CMapFilter::SetTile(int x, int y, int Index, int Flags)
{
	x = clamp(x, 0, m_Width);
	y = clamp(y, 0, m_Height);
	int i = y*m_Width+x;
	m_aFilter[i].m_Index = clamp(Index, (int)CMapFilter::FULL, 255);
	m_aFilter[i].m_Flags = Flags;
}

void CMapFilter::Clear()
{
	for(int i = 0; i < m_aFilter.size(); i++)
		m_aFilter[i].Clear();
	m_aPatterns.clear();
}

void CMapFilter::AddPattern()
{
	float TotalWeight = 0.f;
	for(int i = 0; i < m_aPatterns.size(); i++)
		TotalWeight += m_aPatterns[i].m_Weight;

	int p = m_aPatterns.add(CPattern());
	m_aPatterns[p].SetSize(m_Width, m_Height);
	m_aPatterns[p].Clear();
	m_aPatterns[p].m_Weight = 1.0f - TotalWeight;
}

void CMapFilter::RemovePattern(int Id)
{
	m_aPatterns.remove_index(Id);

	// adjust weights
	float TotalWeight = 0.f;
	for(int i = 0; i < m_aPatterns.size(); i++)
		TotalWeight += m_aPatterns[i].m_Weight;

	float Scale = 1.0f/TotalWeight;
	for(int i = 0; i < m_aPatterns.size(); i++)
		m_aPatterns[i].m_Weight *= Scale;
}

void CMapFilter::ClearPattern(int ID)
{
	if(ID < 0 || ID >= m_aPatterns.size())
		return;
	m_aPatterns[ID].Clear();
}

void CMapFilter::SetPatternTile(int ID, int x, int y, int Index, int Flags)
{
	if(ID < 0 || ID >= m_aPatterns.size())
		return;
	m_aPatterns[ID].SetTile(x, y, Index, Flags);
}

void CMapFilter::SetPatternWeight(int Id, float Weight)
{
	if(m_aPatterns.size() == 1) // weight is 1.0 when there's only one
		return;

	Id = clamp(Id, 0, m_aPatterns.size()-1);
	Weight = clamp(Weight, 0.f, 1.0f);
	m_aPatterns[Id].m_Weight = Weight;

	// adjust other weights
	float OthersTotalWeight = 0.f;
	for(int i = 0; i < m_aPatterns.size(); i++)
	{
		if(i != Id)
			OthersTotalWeight += m_aPatterns[i].m_Weight;
	}

	float Scale = (1.0f-Weight)/OthersTotalWeight;
	for(int i = 0; i < m_aPatterns.size(); i++)
	{
		if(i != Id)
			m_aPatterns[i].m_Weight *= Scale;
	}
}

const array<CMapFilter::CMFTile>& CMapFilter::GetPatternTiles(int ID) const
{
	assert(ID >= 0 && ID < m_aPatterns.size());
	return m_aPatterns[ID].GetTiles();
}

bool CMapFilter::TryApply(int TileID, CLayerTiles* pLayer)
{
	int RefX = m_RefPointId % m_Width;
	int RefY = floor(m_RefPointId / m_Width);

	for(int y = 0; y < m_Height; y++)
	{
		for(int x = 0; x < m_Width; x++)
		{
			int mfi = y*m_Width+x;

			if(mfi == m_RefPointId)
				continue;

			int OffsetX = x - RefX;
			int OffsetY = y - RefY;
			int CheckID = TileID + OffsetY*pLayer->m_Width+OffsetX;

			if(CheckID < 0 || CheckID > pLayer->m_Width*pLayer->m_Height-1)
				return false;

			CTile& Tile = pLayer->m_pTiles[CheckID];
			CMFTile& FTile = m_aFilter[mfi];

			if(FTile.m_Index == CMapFilter::FULL && Tile.m_Index == 0) // full
				return false;
			if(FTile.m_Index == CMapFilter::EMPTY && Tile.m_Index > 0) // empty
				return false;
			if(FTile.m_Index > 0 && (Tile.m_Index != FTile.m_Index || Tile.m_Flags != FTile.m_Flags)) // index specific
				return false;
		}
	}

	// everything checks out, replace
	Apply(TileID, pLayer);

	return true;
}

void CMapFilter::Apply(int TileID, CLayerTiles* pLayer)
{
	if(m_aPatterns.size() == 0)
		return;

	int ChosenID = 0;
	if(m_aPatterns.size() > 1)
	{
		// roll the dice
		float RandVal =  (float)rand() / RAND_MAX;
		ChosenID = m_aPatterns.size()-1;
		float r = 0.f;

		for(int i = 0; i < m_aPatterns.size(); i++)
		{
			r += m_aPatterns[i].m_Weight;
			if(RandVal <= r)
			{
				ChosenID = i;
				break;
			}
		}
	}

	// apply the pattern
	int RefX = m_RefPointId % m_Width;
	int RefY = floor(m_RefPointId / m_Width);
	for(int y = 0; y < m_Height; y++)
	{
		for(int x = 0; x < m_Width; x++)
		{
			int mfi = y*m_Width+x;
			int OffsetX = x - RefX;
			int OffsetY = y - RefY;
			int RepID = TileID + OffsetY*pLayer->m_Width+OffsetX;

			if(RepID < 0 || RepID > pLayer->m_Width*pLayer->m_Height-1)
				continue;

			CTile& Tile = pLayer->m_pTiles[RepID];
			const CMFTile& FTile = m_aPatterns[ChosenID].GetTile(mfi);

			if(FTile.m_Index == CMapFilter::FULL) // leave it as is
				continue;

			Tile.m_Index = FTile.m_Index;
			Tile.m_Flags = FTile.m_Flags;
		}
	}
}


CTilesetMapper_::CTilesetMapper_(CEditor* pEditor):
	IAutoMapper(pEditor, TYPE_TILESET)
{

}

void CTilesetMapper_::Load(const json_value& rElement)
{
	for(unsigned i = 0; i < rElement.u.array.length; ++i)
	{
		if(rElement[i].u.object.length != 1)
			continue;

		// new group
		CGroup Group;
		const char* pConfName = rElement[i].u.object.values[0].name;
		str_copy(Group.m_aName, pConfName, sizeof(Group.m_aName));

		const json_value &rFilterArr = *(rElement[i].u.object.values[0].value);
		if(rFilterArr.type != json_array)
			continue;

		// get filters
		for(unsigned j = 0; j < rFilterArr.u.array.length && j < MAX_RULES; j++)
		{
			CMapFilter Filter(rFilterArr[j]);
			Group.m_aFilters.add(Filter);
		}

		m_aGroups.add(Group);
	}

	for(int i = 0; i < m_aGroups.size(); i++)
	{
		std::cout << "Group: " << m_aGroups[i].m_aName << std::endl;
		for(int f = 0; f < m_aGroups[i].m_aFilters.size(); f++)
			m_aGroups[i].m_aFilters[f].Print();
	}
}

void CTilesetMapper_::Proceed(CLayerTiles* pLayer, int ConfigID)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aGroups.size())
		return;

	CGroup* pGroup = &m_aGroups[ConfigID];

	for(int f = 0; f < pGroup->m_aFilters.size(); f++)
	{
		for(int y = 0; y < pLayer->m_Height; y++)
			for(int x = 0; x < pLayer->m_Width; x++)
			{
				int Index = y*pLayer->m_Width+x;
				if(pLayer->m_pTiles[Index].m_Index > 0)
				{
					if(pGroup->m_aFilters[f].TryApply(Index, pLayer))
						x += pGroup->m_aFilters[f].GetWidth() - 1; // skip ahead if it successfully applied
				}
			}
	}
}

const char* CTilesetMapper_::GetRuleSetName(int Index) const
{
	Index = clamp(Index, 0, m_aGroups.size()-1);
	return m_aGroups[Index].m_aName;
}


//
// CAutoMapUI
//

IGraphics* CAutoMapEd::Graphics()
{
	return m_pEditor->Graphics();
}

ITextRender*CAutoMapEd::TextRender()
{
	return m_pEditor->TextRender();
}

CUI* CAutoMapEd::UI()
{
	return m_pEditor->UI();
}

CRenderTools* CAutoMapEd::RenderTools()
{
	return m_pEditor->RenderTools();
}

CAutoMapEd::CAutoMapEd(CEditor* pEditor)
	: m_UiAlpha(0.85f),
	  m_TabBarHeight(15.f),
	  m_Tileset(pEditor)
{
	m_pEditor = pEditor;
	m_ActiveTab = TAB_FILE;
	m_pImage = 0;
}

CAutoMapEd::~CAutoMapEd()
{
	if(m_pImage)
		delete m_pImage;
}

void CAutoMapEd::Update()
{

}

void CAutoMapEd::Render()
{
	// basic start
	Graphics()->Clear(0.5f, 0.5f, 0.5f);
	CUIRect View = *UI()->Screen();
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);

	/*float Width = View.w;
	float Height = View.h;*/

	// render checker
	//m_pEditor->RenderBackground(View, m_pEditor->m_CheckerTexture, 32.0f, 1.0f);
	m_pEditor->RenderBackground(View, m_pEditor->m_BackgroundTexture, 128.0f, 1.0f);

	// set views
	CUIRect TabBar, ToolBar, LeftPanel;
	View.HSplitTop(m_TabBarHeight, &TabBar, &View);
	View.HSplitTop(52.0f, &ToolBar, &View);
	View.VSplitLeft(100.0f, &LeftPanel, &View);

	// fill views
	RenderTools()->DrawUIRect(&LeftPanel, vec4(0.f, 0.f, 0.f, m_UiAlpha), 0, 0.f);
	RenderTools()->DrawUIRect(&ToolBar, vec4(0.f, 0.f, 0.f, m_UiAlpha), 0, 0.f);
	m_pEditor->RenderBackground(View, m_pEditor->m_CheckerTexture, 32.0f, 1.0f);

	RenderTabBar(TabBar);
	RenderFilterPanel(LeftPanel);

	switch(m_ActiveTab)
	{
	case TAB_FILE:
		RenderFileBar(ToolBar);
	}

	if(m_pEditor->m_Dialog == DIALOG_FILE)
	{
		static int s_NullUiTarget = 0;
		UI()->SetHotItem(&s_NullUiTarget);
		m_pEditor->RenderFileDialog();
	}

	// cursor
	float mx = UI()->MouseX();
	float my = UI()->MouseY();
	Graphics()->TextureSet(m_pEditor->m_CursorTexture);
	Graphics()->QuadsBegin();
	/*if(ms_pUiGotContext == UI()->HotItem())
		Graphics()->SetColor(1,0,0,1);*/
	IGraphics::CQuadItem QuadItem(mx,my, 16.0f, 16.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

int CAutoMapEd::DoButton_MenuTabTop(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	float Aplha = Checked ? m_UiAlpha : m_UiAlpha / 2.f;
	RenderTools()->DrawUIRect(pRect, vec4(0.f, 0.f, 0.f, Aplha), /*CUI::CORNER_T*/0, 5.f);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.25f);

	const float FontSize = 8.f;
	UI()->DoLabel(pRect, pText, FontSize, CUI::ALIGN_CENTER);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

void CAutoMapEd::RenderTabBar(CUIRect &Box)
{
	CUIRect Button;

	const float Spacing = 0.f;
	const float TabNum = 10.f;
	const float ButtonWidth = (Box.w/TabNum)-(Spacing*5.f)/TabNum;

	Box.HSplitBottom(m_TabBarHeight, 0, &Box);

	Box.VSplitLeft(ButtonWidth, &Button, &Box);
	static int s_TabFileButton = 0;
	if(DoButton_MenuTabTop(&s_TabFileButton, "File", (m_ActiveTab == TAB_FILE), &Button))
	{
		m_ActiveTab = TAB_FILE;
	}

	Box.VSplitLeft(Spacing, 0, &Box); // little space
	Box.VSplitLeft(ButtonWidth, &Button, &Box);
	static int s_TabToolsButton = 0;
	if(DoButton_MenuTabTop(&s_TabToolsButton, "Tools", (m_ActiveTab == TAB_TOOLS), &Button))
	{
		m_ActiveTab = TAB_TOOLS;
	}
}

void CAutoMapEd::RenderFileBar(CUIRect& Box)
{
	CUIRect TB_Top, TB_Bottom;
	CUIRect Button;

	Box.HSplitTop(Box.h/2.0f, &TB_Top, &TB_Bottom);

	/*TB_Top.HSplitBottom(8.f, &TB_Top, 0);
	TB_Bottom.HSplitTop(8.f, 0, &TB_Bottom);*/
	TB_Top.Margin(2.0f, &TB_Top);
	TB_Bottom.Margin(2.0f, &TB_Bottom);

	// open button
	TB_Top.VSplitLeft(40.0f, &Button, &TB_Top);
	static int s_OpenButton = 0;
	if(m_pEditor->DoButton_Ex(&s_OpenButton, "Open", 0, &Button, 0, "[CTRL+O] Open/Create an automap file", CUI::CORNER_ALL))
	{
		m_pEditor->InvokeFileDialog(IStorage::TYPE_ALL, CEditor::FILETYPE_IMG, "Open Image", "Open", "mapres", "", OpenImage, this);
	}

	TB_Top.VSplitLeft(5.0f, 0, &TB_Top);

	// save button
	TB_Top.VSplitLeft(40.0f, &Button, &TB_Bottom);
	static int s_SaveButton = 0;
	if(m_pEditor->DoButton_Ex(&s_SaveButton, "Save", 0, &Button, 0, "[CTRL+S] Save file", CUI::CORNER_ALL))
	{

	}
}

void CAutoMapEd::RenderFilterPanel(CUIRect& Box)
{
	CUIRect Title;
	Box.HSplitTop(15.f, &Title, &Box);

	RenderTools()->DrawUIRect(&Title, vec4(0.25f, 0.25f, 0.25f, 1.0f), 0, 0.f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.25f);

	const float FontSize = 10.f;
	UI()->DoLabel(&Title, "Filters", FontSize, CUI::ALIGN_CENTER);
}

void CAutoMapEd::OpenImage(const char *pFileName, int StorageType, void *pUser)
{
	CAutoMapEd* pAMEd = (CAutoMapEd *)pUser;
	CEditor* pEditor = pAMEd->m_pEditor;
	CEditorImage* pImg = new CEditorImage(pEditor);
	if(!pEditor->Graphics()->LoadPNG(pImg, pFileName, StorageType))
		return;

	char aBuf[128];
	pEditor->ExtractName(pFileName, aBuf, sizeof(aBuf));
	str_copy(pImg->m_aName, aBuf, sizeof(pImg->m_aName));

	pImg->m_Texture = pEditor->Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, pImg->m_Format, pImg->m_pData, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);
	pImg->m_pData = 0;

	if(pAMEd->m_pImage)
		delete pAMEd->m_pImage;
	pAMEd->m_pImage = pImg;

	// load the coresponding automapper file
	pAMEd->m_pImage->LoadAutoMapper();

	pEditor->m_Dialog = DIALOG_NONE;
}

