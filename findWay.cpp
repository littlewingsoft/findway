#include "dxut.h"

#include <string>
#include <map>

#include "findWay.h"
#include "./tinyXml/tinyXml.h"



/*
Heuristic + ArrivalCost 입니다. 
 
여기서 Heuristic 은
deltax = (목표점.x - 현제셀중점.x)
deltay = (목표점.y - 현제셀중점.y)
deltaz = (목표점.z - 현제셀중점.z)
max(max(deltax, deltay), deltaz)

으로 계산.

어느 x,z, 를 가지고 y 를 알고싶다면 
평면의 방정식( 노멀.x, 노멀.y,노멀.z , 거리 ) 를 구해놓자. 거리는 축의원점과 삼각형의원점간 offset 일까?

closeList 와 openList 를 만들자.

closeList : 이미 처리된것.?
openList. 

[출처] 네비게이션 메쉬 + A* (Navigation Mesh + AStar)|작성자 지노윈



*/
namespace fw
{
	// 곧 지우던가 아니면 node 처리가 되면 정리할것들.
	static fwMesh g_AgentMesh;
	static fwMesh g_PointMesh;

	D3DXVECTOR3 camEyePos,camLookAt;
	float fcamFov;
	float fNear, fFar;


	// 네비메쉬 담아놓는것.
	static fwMesh g_NaviMesh;


	struct testCode
	{
		testCode()
		{
			fw::NaviMesh_LoadXml( "default.xml" );
		}
	};
	static testCode tt;	

	bool MakeMesh( TiXmlElement* pkElem, fwMesh & mesh )
	{
		if( pkElem == 0 ) 
			return false;

		D3DXCOLOR wireClr( 1,1,1,0.5f );
		{ // vtx중 pos 만 추출
			TiXmlElement* pkFirstElem = pkElem->FirstChildElement( "WIRECOLOR" );
			const char* value = pkFirstElem->GetText();
			int r=0,g=0,b=0;
			sscanf_s( value, "%d,%d,%d", &r,&g,&b );
			wireClr = D3DXCOLOR( r/255.0f,g/255.0f,b/255.0f, 1.0f );
		}


		// 실제 메쉬그룹검색완료. 여기서 버텍스와 인덱스 값을 가지고 삼각형 만들면됨.
		{ // vtx중 pos 만 추출
			TiXmlNode* pkParent = pkElem->FirstChild( "Vertex" );
			TiXmlElement* pkPosElem = pkParent ->FirstChildElement();
			while( pkPosElem )
			{
				const char* value = pkPosElem->GetText();
				float x=0.0f,y=0.0f,z=0.0f;
				sscanf_s( value, "[%f,%f,%f]", &x,&y,&z );
				fw::Vertex vtx;
				vtx.pos = D3DXVECTOR3( x,y,z );
				vtx.Color = wireClr; //D3DXCOLOR( 128,128,128,0xff ); //(rand()%256) / 256.0f, (rand()%256) / 256.0f,(rand()%256) / 256.0f
				mesh.VtxBuffer.push_back( vtx );
				pkPosElem= pkPosElem->NextSiblingElement();
			}
		}



		{
			TiXmlNode* pkTMParent= pkElem->FirstChild( "LocalTM" );
			TiXmlElement* pkTmpElem = pkTMParent->FirstChildElement( "Value" );

			sscanf_s( pkTmpElem->GetText(), "[%f,%f,%f,%f]", &mesh.LocalMat._11,&mesh.LocalMat._12,&mesh.LocalMat._13, &mesh.LocalMat._14);
			pkTmpElem = pkTmpElem ->NextSiblingElement();

			sscanf_s( pkTmpElem->GetText(), "[%f,%f,%f,%f]", &mesh.LocalMat._21,&mesh.LocalMat._22,&mesh.LocalMat._23, &mesh.LocalMat._24);
			pkTmpElem = pkTmpElem ->NextSiblingElement();

			sscanf_s( pkTmpElem->GetText(), "[%f,%f,%f,%f]", &mesh.LocalMat._31,&mesh.LocalMat._32,&mesh.LocalMat._33, &mesh.LocalMat._34);
			pkTmpElem = pkTmpElem ->NextSiblingElement();

			sscanf_s( pkTmpElem->GetText(), "[%f,%f,%f,%f]", &mesh.LocalMat._41,&mesh.LocalMat._42,&mesh.LocalMat._43, &mesh.LocalMat._44);
		}

		{ // TriIndex 추출.
			TiXmlNode* pkParent = pkElem->FirstChild( "TriIndex" );
			TiXmlElement* pkTriElem = pkParent ->FirstChildElement();
			while( pkTriElem )
			{
				fw::Triangle tri;
				const char* value = pkTriElem ->GetText();
				sscanf_s( value, "[%d,%d,%d]", &tri.index0,&tri.index1,&tri.index2);

				mesh.TriBuffer.push_back( tri );
				pkTriElem = pkTriElem ->NextSiblingElement();
			}
		}

		{
			TiXmlNode* pkParent = pkElem->FirstChild( "NaviCell" );
			if( pkParent )
			{
				TiXmlElement* pkTriElem = pkParent ->FirstChildElement();

				while( pkTriElem )
				{
					fw::fwNaviCell cell;
					int triIndex = 0;
					pkTriElem->Attribute( "TriIndex", &triIndex  );
					const char* szCenter = pkTriElem->Attribute( "Center" );
					sscanf_s( szCenter, "[%f,%f,%f]", &cell.center.x,&cell.center.y,&cell.center.z );

					const char* szNeighbor = pkTriElem->Attribute( "Neighbor" );
					sscanf_s( szNeighbor, "(EdgeIndex XY:%d YZ:%d ZX:%d)", &cell.NeighborTri[0], &cell.NeighborTri[1], &cell.NeighborTri[2] );

					TiXmlElement* pkChildElem = pkTriElem->FirstChildElement("EdgeCenter");
					pkChildElem = pkChildElem->FirstChildElement("AB" );

					sscanf_s( pkChildElem->GetText(), "[%f, %f, %f] ( %f )", &cell.edgeCenter[0].x, &cell.edgeCenter[0].y, &cell.edgeCenter[0].z, &cell.arrivalCost[0] );

					pkChildElem = pkChildElem->NextSiblingElement("BC");
					sscanf_s( pkChildElem->GetText(), "[%f, %f, %f] ( %f )", &cell.edgeCenter[1].x, &cell.edgeCenter[1].y, &cell.edgeCenter[1].z, &cell.arrivalCost[1] );

					pkChildElem = pkChildElem->NextSiblingElement("CA");
					sscanf_s( pkChildElem->GetText(), "[%f, %f, %f] ( %f )", &cell.edgeCenter[2].x, &cell.edgeCenter[2].y, &cell.edgeCenter[2].z, &cell.arrivalCost[2] );

					mesh.CellBuffer.push_back( cell );

					pkTriElem = pkTriElem ->NextSiblingElement();
				}
			}			
		
		}

		return true;
	}

	int NaviMesh_LoadXml(const std::string& name )
	{
		//xmlDocument 로딩하기.
		TiXmlDocument doc;
		doc.LoadFile( name.c_str() );
		g_NaviMesh.TriBuffer.clear();
		TiXmlNode* pkNode = doc.FirstChild( "Body" );
		if( pkNode )
		{

			pkNode = pkNode ->FirstChildElement( "Object" );
			TiXmlElement* pkElem = pkNode ->ToElement();
			while( pkElem )
			{
				std::string strClass = pkElem ->Attribute( "Class" );

				if( strClass == "Editable_mesh" )
				{
					std::string strName = pkElem ->Attribute( "Name" );
					if( strName == "navi_ground" )
					{
						MakeMesh( pkElem, g_NaviMesh );
					}
					else if( strName == "agent" )
					{
						MakeMesh( pkElem, g_AgentMesh );
					}
					else if( strName == "point" )
					{
						MakeMesh( pkElem, g_PointMesh );
					}

				}
				else if( strClass == "Targetcamera" )
				{
					TiXmlNode* pkTMParent= pkElem->FirstChild( "LocalTM" );
					TiXmlNode* pkTmpNode = pkTMParent->FirstChild( "Value" );
					pkTmpNode= pkTmpNode->NextSibling();
					pkTmpNode= pkTmpNode->NextSibling();
					TiXmlElement* pkTmpElem = pkTmpNode->NextSiblingElement();

					float fTmp;
					const char* value = pkTmpElem->GetText();
					sscanf_s( value, "[%f,%f,%f,%f]", &camEyePos.x,&camEyePos.y,&camEyePos.z, &fTmp );

					{
						TiXmlElement* pkTMParent= pkElem->FirstChildElement( "FOV" );
						const char* value = pkTMParent->GetText();
						sscanf_s( value, "[%f]", &fcamFov );
					}

					{
						TiXmlElement* pkTMParent= pkElem->FirstChildElement( "NEAR" );
						const char* value = pkTMParent->GetText();
						sscanf_s( value, "[%f]", &fNear );					
					}

					{
						TiXmlElement* pkTMParent= pkElem->FirstChildElement( "FAR" );
						const char* value = pkTMParent->GetText();
						sscanf_s( value, "[%f]", &fFar );					
					}
				}
				else if( strClass == "Targetobject" )
				{
					TiXmlNode* pkTMParent= pkElem->FirstChild( "LocalTM" );
					TiXmlNode* pkTmpNode = pkTMParent->FirstChild( "Value" );
					pkTmpNode= pkTmpNode->NextSibling();
					pkTmpNode= pkTmpNode->NextSibling();
					TiXmlElement* pkTmpElem = pkTmpNode->NextSiblingElement();

					float fTmp;
					const char* value = pkTmpElem->GetText();
					sscanf_s( value, "[%f,%f,%f,%f]", &camLookAt.x,&camLookAt.y,&camLookAt.z, &fTmp );
				}


				pkElem = pkElem->NextSiblingElement();
			}
		}


		return 0;
	}

	void NaviMesh_Release()
	{

	}

	const fwMesh & GetNaviMesh()
	{
		return g_NaviMesh;
	}

	fwMesh & GetAgentMesh()
	{
		return g_AgentMesh;
	}

	fwMesh & GetPointMesh()
	{
		return g_PointMesh;
	}


	
//bool IntersectTriangle( const D3DXVECTOR3& org, const D3DXVECTOR3& dir, D3DXVECTOR3& v0, D3DXVECTOR3& v1, D3DXVECTOR3& v2, float* fDistance, float* u, float* v )
//{
//	// Find vectors for two edges sharing vert0
//	D3DXVECTOR3 edge1 = v1 - v0;
//	D3DXVECTOR3 edge2 = v2 - v0;
//
//	// Begin calculating determinant - also used to calculate U parameter
//	D3DXVECTOR3 pvec;
//	D3DXVec3Cross( &pvec, &ray.vPickRayDir, &edge2 );
//
//	// If determinant is near zero, ray lies in plane of triangle
//	FLOAT det = D3DXVec3Dot( &edge1, &pvec );
//
//	D3DXVECTOR3 tvec;
//	if( det > 0 )
//	{
//		tvec = ray.vPickRayOrig - v0;
//	}
//	else
//	{
//		tvec = v0 - ray.vPickRayOrig;
//		det = -det;
//	}
//
//	if( det < 0.0001f )
//		return false;
//
//	// Calculate U parameter and test bounds
//	*u = D3DXVec3Dot( &tvec, &pvec );
//	if( *u < 0.0f || *u > det )
//		return false;
//
//	// Prepare to test V parameter
//	D3DXVECTOR3 qvec;
//	D3DXVec3Cross( &qvec, &tvec, &edge1 );
//
//	// Calculate V parameter and test bounds
//	*v = D3DXVec3Dot( &ray.vPickRayDir, &qvec );
//	if( *v < 0.0f || *u + *v > det )
//		return false;
//
//	// Calculate t, scale parameters, ray intersects triangle
//	*fDistance = D3DXVec3Dot( &edge2, &qvec );
//	FLOAT fInvDet = 1.0f / det;
//	*fDistance *= fInvDet;
//	*u *= fInvDet;
//	*v *= fInvDet;
//
//	return true;
//}


	bool InterSectTriangle(const D3DXVECTOR3& m, const D3DXVECTOR3& p0,const D3DXVECTOR3& p1, const D3DXVECTOR3& p2 )
	{
		////외적을 이용해 노말을 구한다.
		D3DXVECTOR3 v0 = p1 - p0;
		D3DXVECTOR3 v1 = p2 - p0;
		D3DXVECTOR3 Nomal;
		D3DXVec3Cross(&Nomal, &v0, &v1);
		D3DXVec3Normalize(&Nomal, &Nomal);

		////3평면과 3개의 노말을 정의한다.
		D3DXPLANE Plane1;
		D3DXVECTOR3 n1;
		D3DXPLANE Plane2;
		D3DXVECTOR3 n2;
		D3DXPLANE Plane3;
		D3DXVECTOR3 n3;

		////두점과 묶어줄 한점을 구한다.
		n1 = (p0 + p2) / 2.0f + Nomal;
		n2 = (p0 + p1) / 2.0f + Nomal;
		n3 = (p2 + p1) / 2.0f + Nomal;


		////두점과 위의 노말값으로 평면을 만든다.
		D3DXPlaneFromPoints(&Plane1, &p2, &p0, &n1);
		D3DXPlaneFromPoints(&Plane2, &p0, &p1, &n2);
		D3DXPlaneFromPoints(&Plane3, &p1, &p2, &n3);

		float d1, d2, d3;

		////평면의 방정식에 대입한다.

		//// ax + by + cz +d < 0 으면 밖에 있다.

		//// m.x m.y m.z 는 점의 좌표입니다.
		d1 = m.x * Plane1.a + m.y * Plane1.b + m.z * Plane1.c + Plane1.d;
		d2 = m.x * Plane2.a + m.y * Plane2.b + m.z * Plane2.c + Plane2.d;
		d3 = m.x * Plane3.a + m.y * Plane3.b + m.z * Plane3.c + Plane3.d;

		////d1 d2 d3가 전부다 0보다 크면 평면내부에 있다.
		if( d1 > 0 && d2 > 0 && d3 > 0 )
			return true;

		return false;
	}


	void FindWay( const int triIndex, const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList )
	{
		pathList.clear();
		//NaviCell fw::GetNaviCell( triIndex )
		
	}
	// 시작위치와 끝위치를 넣으면 pathList 가 나온다.
	// 만일 시간이오래 걸리면 비동기 처리를 해야 하려나.
	// 일단은 매우 최대한 간단하고 simple 하게 유지함.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList )
	{
		pathList.clear();

		for( size_t n=0; n< fw::GetNaviMesh().TriBuffer.size(); n++)
		{
			fw::Triangle triIndex = fw::GetNaviMesh().TriBuffer[n];

			D3DXVECTOR3 V0 = fw::GetNaviMesh().VtxBuffer[ triIndex.index0 ].pos;
			D3DXVECTOR3 V1 = fw::GetNaviMesh().VtxBuffer[ triIndex.index1 ].pos;
			D3DXVECTOR3 V2 = fw::GetNaviMesh().VtxBuffer[ triIndex.index2 ].pos;

			D3DXVECTOR3 wV0,wV1,wV2;
			// 로컬좌표에서 이루어진 메시정보를 월드좌표로 변환
			D3DXVec3TransformCoord(&wV0, &V0, &fw::GetNaviMesh().LocalMat  );
			D3DXVec3TransformCoord(&wV1, &V1, &fw::GetNaviMesh().LocalMat);
			D3DXVec3TransformCoord(&wV2, &V2, &fw::GetNaviMesh().LocalMat);

			//picking polygon IndexList 를 가지고 있다가 최고 적은 크기 값을 유지시킨다.
			//float fBary2=0.0f,fBary1=0.0f,fDist=0.0f;
			//if( InterSectTriangle( start_pos, V0, V1, V2 ) )
			{
			//	break;
			}
		}


	}

};
