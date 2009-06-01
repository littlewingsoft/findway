#include "dxut.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <queue>

#include "findWay.h"
#include "./tinyXml/tinyXml.h"

using namespace std;

/*
4. Heap 만들기
heap은 End좌표로 부터 거꾸로 start까지의 셀 노드들의 리스트 입니다.
end cell은 시작점, start cell이 목표점으로 생각합니다.
EndCell로 부터 인접한 셀들을 구합니다. 이 인접 셀들중 비용이 가장 싼 것을 계산 합니다. 계산식은 Heuristic + ArrivalCost 입니다. 
 
여기서 Heristic은
deltax = (목표점.x - 현제셀중점.x)
deltay = (목표점.y - 현제셀중점.y)
deltaz = (목표점.z - 현제셀중점.z)
max(max(deltax, deltay), deltaz)
값입니다.
 
이웃셀 셀중 비용이 싼 셀이 현제셀로 되며 같은 방식으로 다시 이웃셀 셀을 비교 합니다.
이를 반복 하면 start 셀까지 셀이 이동 됩니다.
 
5. Path 만들기
앞에서 만든 heap으로 부터 start로 부터 end까지의 mid좌표와 cell을 연속적으로 저장해 둡니다.
 
6. 추가 작업
삼각형의 중점으로 Cell들을 이동해 다니게 되면 술취한(갈지자) Actor처럼 보이므로, 앞에서 구해진 Path로 부터 line테스트를 하여 직선 경로를 구해 이를 이동 경로로 사용 합니다.
[출처] 네비게이션 메쉬 + A* (Navigation Mesh + AStar)|작성자 지노윈

끝점(루프시작)->  시작점(루프끝)

*/






namespace fw
{
	class exc : public exception 
	{
	public:
		exc( const std::string szStr )
		{
			MessageBoxA(0, "exception", szStr.c_str() ,MB_OK);
			__asm int 3;
		}

		exc(const wstring szStr )
		{
			MessageBoxW(0, L"exception", szStr.c_str() ,MB_OK);
			__asm int 3;
		}
	};

	// 곧 지우던가 아니면 node 처리가 되면 정리할것들.
	D3DXVECTOR3 camEyePos,camLookAt;
	float fcamFov;
	float fNear, fFar;

	static std::map<string,fwMesh> g_MeshList;



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

		// 광원위한 노멀 계산.
		{ 
			int n=0;
			TiXmlNode* pkParent = pkElem->FirstChild( "VertexNormal" );
			TiXmlElement* pkPosElem = pkParent ->FirstChildElement();
			while( pkPosElem )
			{
				const char* value = pkPosElem->GetText();
				float x=0.0f,y=0.0f,z=0.0f;
				sscanf_s( value, "[%f,%f,%f]", &x,&y,&z );
				mesh.VtxBuffer[ n ].norm = D3DXVECTOR3( x,y,z );
				pkPosElem= pkPosElem->NextSiblingElement();
				n++;
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

					D3DXVECTOR3 normal,n0,n1,n2;
					n0 =mesh.VtxBuffer[ mesh.TriBuffer[ triIndex].index0  ].norm; // 이건 좀아니잖아.
					n1 =mesh.VtxBuffer[ mesh.TriBuffer[ triIndex].index1  ].norm; // 이건 좀아니잖아.
					n2 =mesh.VtxBuffer[ mesh.TriBuffer[ triIndex].index2  ].norm; // 이건 좀아니잖아.
					cell.normal = (n0+n1+n2 )/3.0f;
					D3DXVec3Normalize( &cell.normal, &cell.normal );
					// 그냥여기서 일단 노멀계산???

//					const char* szNeighbor = pkTriElem->Attribute( "Neighbor" );
//					sscanf_s( szNeighbor, "(EdgeIndex XY:%d YZ:%d ZX:%d)", &cell.NeighborTri[0], &cell.NeighborTri[1], &cell.NeighborTri[2] );

					TiXmlElement* pkChildElem = pkTriElem->FirstChildElement("EdgeCenter");
					pkChildElem = pkChildElem->FirstChildElement();

					for( int n=0;n<3; n++) // 이웃 3개의 정보를 셋팅.
					{
						const char* szNeighborIndex = pkChildElem->Attribute("NeighborFaceIndex");
						cell.edge[n].NeighborIndex = atoi( szNeighborIndex );
						sscanf_s( pkChildElem->GetText(), "[%f, %f, %f] ( %f )", 
								  &cell.edge[n].center.x, &cell.edge[n].center.y, 
								  &cell.edge[n].center.z, &cell.edge[n].arrivalCost);

						pkChildElem = pkChildElem->NextSiblingElement();
					}

					mesh.CellBuffer.push_back( cell );

					pkTriElem = pkTriElem ->NextSiblingElement();
				}
			}			

		}

		return true;
	}


	void RemoveMesh( const std::string& meshName )
	{

	}

	void RemoveAllMesh()
	{
		g_MeshList.clear();
	}


	int AddMesh_FromXml(const std::string& xmlname )
	{
		TiXmlDocument doc;
		if( doc.LoadFile( xmlname.c_str() ) == false )
		{
			
			throw exc( L"xml파일을 찾을수없습니다.");
		}

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
					std::transform( strName.begin(), strName.end(), strName.begin(), tolower );
					fwMesh mesh;
					MakeMesh( pkElem, mesh );
					g_MeshList.insert( std::pair<string, fwMesh>(strName,mesh) );
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





	const fwMesh& GetNaviMesh()
	{
		const std::string szCONVENTION_NAVI = "navimesh";
		return GetMesh( "navimesh" );
		/*map< string, fwMesh >::iterator it = g_MeshList.begin();
		
		while( it != g_MeshList.end() )
		{
			const string& meshName = (*it).first;
			size_t i = meshName.find_first_not_of( szCONVENTION_NAVI );
			if( i == szCONVENTION_NAVI.length()  )
			{
				return GetMesh( meshName );
			}

			it++;
		}
		throw exc("");
		*/
	}


	inline fwMesh& GetMesh(const string&name )
	{
		std::string strName = name;
		//std::transform( strName.begin(), strName.end(), strName.begin(), tolower );
		
		if( g_MeshList.count( strName ) == 0 )
		{
			char  buff[64]={0,};
			sprintf_s( buff,64, "%s를 찾을수없음", strName.c_str() );
			throw exc( buff );
		}else
		{
			return g_MeshList[strName];
		}

	}


	const fwMesh& GetMesh_const(const string& name ) 
	{
		return GetMesh( name );
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

	float ComputeHeuristic( const D3DXVECTOR3& currentCenter, const D3DXVECTOR3& goal )
	{
		float deltaX = fabs(goal.x - currentCenter.x);
		float deltaY = fabs(goal.y - currentCenter.y);
		float deltaZ = fabs(goal.z - currentCenter.z);
		float heuristic = max( max( deltaX, deltaY ), deltaZ );
		return heuristic ;
	}

	//priority_Vector
	fwPathHeap g_PathHeap;
	static std::map<int, fwPathHeapNode> visitedNodeList; //이미 처리 끝난것.

	/*
		 현재 쎌의 모든 이웃셀을 찾는다. 
		 if ( 오픈리스트.찾기(이웃셀) == 있다 )
		 { 있으면 그냥 넘어가. }
		 else
		 { 인덱스를 오픈리스트에 넣는다. }	
	*/
	int FindSmallestHeuristicCell_AddOpenList( const fwPathHeapNode& topNode, const D3DXVECTOR3& endPos )
	{
		const fwMesh& kMesh = GetNaviMesh();
//		fwPathHeapNode topNode ;
//		g_PathHeap.Top( topNode );			

		const fwNaviCell& focusCell= kMesh.CellBuffer[ topNode.kCurrentCell_Index ];

		for( int indexCnt= 0; indexCnt<3; indexCnt++ )
		{
			int neighborIndex = focusCell.edge[ indexCnt ].NeighborIndex;
			int cnt = visitedNodeList.count( neighborIndex );
			if( neighborIndex == -1 || cnt != 0 )  // 이웃이 있어야함.
				continue;

			// 부모로부터 내위치까지의거리. 
			// 현재셀이 부모가 되고 이웃셀은 이때 현재셀이된다.
			///float G_costFromParent = 0.0f;
			float G_costFromStart =  topNode.costFromStart+ focusCell.edge[indexCnt].arrivalCost ;//현재셀까지 누적된값 + 이웃셀 의코스트.
			float H_costToGoal = ComputeHeuristic( focusCell.edge[ indexCnt ].center , endPos );//neighborCell.center 
			fw::fwPathHeapNode newNode( topNode.kCurrentCell_Index , neighborIndex, indexCnt, G_costFromStart ,H_costToGoal );

			g_PathHeap.AddPathNode( newNode ); // 여기서 추가된것체크를 한뒤에 추가가 된상태면 값만 갱신한다.

		}


		return -1;
	}




/**



용어
열린목록 : 가능성이 있는 지점을 저장한 목록.
닫힌목록 : 가능성이 없는 지점을 저장한 목록. 따라서 다시는 볼 필요가 없다.


정리
열린목록 : 우선순위 큐 (F=G+H 값이 작을수록 높은 우선순위)

열린목록을 우선순위 큐로 할 상황이 아니거나, 다른 자료구조로 할 필요성이 생겼을 때는 아래 알고리즘에서 열린목록.뺌 을 다음과 같이 이해하면 된다

열린목록.뺌 - 열린목록 중 F=G+H 값이 가장 작은 값을 빼오면 된다

닫힌목록 : 어떠한 자료구조 무관할 듯(속도를 위해서라면 map이겠지)

 
1) 열린목록.추가(시작지점)
2) 반복
{
  1) 현재셀 = 열린목록.뺌()
  2) 닫힌목록.푸시(현재셀)
  3) foreach(이웃셀 in 현재셀 인접)
  {
    1) if(이웃셀.갈수있음() && not 닫힌목록.포함(이웃셀))

      1) if(not 열린목록.포함(이웃셀))

        1) 열린목록.추가(이웃셀)
        2) 이웃셀.부모 = 현재셀
        3) 이웃셀.F비용계산() | 방법 >> F = G(이웃셀.부모.F비용) + H(GOAL까지 거리) | 이웃셀.부모 는 현재지점이므로 G 를 현재셀.F비용으로 하면 됨

      2) else // if(열린목록.포함(이웃셀))

        1) 이웃셀.G비용계산() | 방법 >> 현재비용.F비용(이웃셀.부모 는 지정안되어있있으므로 이웃셀.부모.F비용으로 하면 안됨)
        2) 계산된 비용이 더 작으면

          1) 이웃셀.부모 = 현재셀
          2) 이웃셀.F비용계산() | 방법 >> F = G(이웃셀.부모.F비용) + H(GOAL까지 거리) | 이웃셀.부모 는 현재지점이므로 G 를 현재셀.F비용으로 하면 됨

  }

    2) 목표지점을 열린목록에 추가했다면, loop 빠져나감
    3) 열린목록이 비었다면, 찾는데 실패했으므로 loop 빠져나감
}

3) 길저장하기 | 방법 >> 목표지점으로 부터 각 부모지점을 따라가다, 첫지점이 나올 때까지 스택에 저장하라.
4) 길따라가기 | 방법 >> 스택에서 하나씩 pop() 하면 된다.
	*/

	D3DXVECTOR3 findNeighborEdgeCenter( const fw::fwPathHeapNode& tmpnode )
	{
		fw::fwNaviCell Cell = fw::GetNaviMesh().CellBuffer[ tmpnode.kParentCell_Index ];
		//fw::fwNaviCell Cell = fw::GetNaviMesh().CellBuffer[ tmpnode.kCurrentCell_Index ];
		//if( tmpnode.kCurrentCell_Index != -1 )
		{
			
		}
		return Cell.edge[tmpnode.kNeighborEdgeIndex].center;
	}

	D3DXVECTOR3 deprecated_findNeighborEdgeCenter( const fw::fwPathHeapNode& tmpnode )
	{
		fw::fwNaviCell parentCell = fw::GetNaviMesh().CellBuffer[ tmpnode.kParentCell_Index ];
		fw::fwNaviCell focusCell = fw::GetNaviMesh().CellBuffer[ tmpnode.kCurrentCell_Index ];

		float fLastMin =  3.4E+38f;
		D3DXVECTOR3 pos(0,0,0);
		for( int n=0; n<3; n++)
		{
			if( focusCell.edge[n].NeighborIndex == -1 )
				continue;

			//edge 데이터가 잘몬되었다.
			D3DXVECTOR3 tmp= parentCell.center - focusCell.edge[n].center;
			float fMin = D3DXVec3Length( &tmp );
			if( fMin < fLastMin )
			{
				pos = focusCell.edge[n].center;
				fLastMin = fMin;
			}
		}	

		return pos;
	}

	void buildPath( const int endCellIndex,  const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos , std::vector< fwPathNode > & g_pathList )
	{
		// 최종적으로 목적지에 닿았다면 그것이 top 이 될것이다.
		// 그것의 부모노드를 차곡차곡 찾아가자.
		fwPathNode tmpPathNode;
		tmpPathNode.cell_Index=endCellIndex;
		tmpPathNode.pos = end_pos ;
		g_pathList.push_back( tmpPathNode );

		if( visitedNodeList.empty() || visitedNodeList.count( endCellIndex ) == 0 )
			return;


		fwPathHeapNode tmpHeapNode = visitedNodeList[endCellIndex];
		while( tmpHeapNode.kParentCell_Index != - 1 )
		{//부모노드가 시작노드와 같을때까지 계속 찾기.시작노드만 부모가 -1 로 셋팅되있음.

			// center 로 하면 문제가 있음. 직선거리가 되버림. edge 를 꼭 거쳐야함.
			// 자신의 edge 중에서 부모의 중심과 가장 가까운 edge를 찾는다.

			const D3DXVECTOR3& pos = findNeighborEdgeCenter( tmpHeapNode );

			tmpPathNode.pos = pos;
			tmpPathNode.cell_Index = tmpHeapNode.kCurrentCell_Index;

			g_pathList.push_back( tmpPathNode );

			tmpHeapNode = visitedNodeList[ tmpHeapNode.kParentCell_Index];

		}

		tmpPathNode.cell_Index = tmpHeapNode.kCurrentCell_Index;
		tmpPathNode.pos = start_pos;
		// 보간될 길을 모두 찾았으면 start_pos 가 최종위치임.
		g_pathList.push_back( tmpPathNode );

	}

	bool IsSameNormal( const int cellIndexA, const int cellIndexB )
	{
		const fwMesh& kMesh = GetNaviMesh();

		const fwNaviCell& cellA = kMesh.CellBuffer[ cellIndexA ];
		const fwNaviCell& cellB = kMesh.CellBuffer[ cellIndexB ];

		D3DXVECTOR3 vOut;
		D3DXVec3Cross( &vOut, &cellA.normal, &cellB.normal );

		float len = D3DXVec3Length( &vOut );
		if( len >= 0.1f ) // 선분이 조금이라도 각이 다르면 
			return false;

		return true;
	}

	bool IsShowEachOther( int pinFocus, int testFocus )
	{
		return true;
	}

	void Optimize_OmitPath( std::vector< fwPathNode > & pathList )
	{
		if( pathList.size() <= 2 ) //2개이하면 계산 안함.
			return; 

		vector<fwPathNode>::iterator it = pathList.begin();
		
		it+=1; // 시작+1 은 같은 삼각형 인덱스이므로 건너뜀.
		int pinFocus = (*it).cell_Index; // 테스트기준이 되는 포커스인덱스

		it+=1;
		int testFocus = (*it).cell_Index; // 테스트 대상 포커스인덱스
		
		
		while( it != pathList.end() )
		{
			if( IsSameNormal( pinFocus, testFocus ) && IsShowEachOther( pinFocus, testFocus ) )
			{//같은 기울기의 평면이어야 하고, 서로 중점간 연결했을때 열린엣지를 통해야 한다.
				(*it).cell_Index = -1; //테스트 포커스는 빼도되는 무효한인덱스로 설정.
				// 무효한 애들은 한방에 다 빼버림.
	
			}
			else
			{
				pinFocus = testFocus;
			}

			it++;
				if( it == pathList.end() )
					break;
				testFocus = (*it).cell_Index;

			it++;
		}
		
		it = pathList.begin();
		while( it != pathList.end() )
		{
			if( (*it).cell_Index == -1 )
				it = pathList.erase( it );
			else
			it++;
		}

	}
	
	void Optimize_FineTuning( std::vector<fwPathNode>& pathList )
	{
	
	}

	// 이미 외부에서는 반직선 값만 넘겨주게 해야 될듯. 피킹으로 삼각형(셀) 인덱스를 찾아서 넘겨줬다. 
	// 일단 g_pathList 는 최단거리 최적화는 하지않는다.
	void FindWay(	const int _startCellIndex, const D3DXVECTOR3& _start_pos, 
					const int _endCellIndex, const D3DXVECTOR3& _end_pos , std::vector< fwPathNode > & pathList )
	{
		int startCellIndex =_endCellIndex;
		const D3DXVECTOR3& start_pos= _end_pos;
		const int endCellIndex = _startCellIndex;
		const D3DXVECTOR3& end_pos =   _start_pos;

		pathList.clear();
		g_PathHeap.clear();
		visitedNodeList.clear();

		// 새로운 이웃셀을 체크할때 여기에 있는지 체크해본뒤 있다면 건너뜀.
		int  currCell = startCellIndex;
		fwPathHeapNode node( -1, startCellIndex, 0, -1, ComputeHeuristic( start_pos, end_pos ) );
		g_PathHeap.AddPathNode( node );

		if( currCell == endCellIndex  )
		{
			fwPathNode node;

			node.pos = start_pos;
			node.cell_Index=_startCellIndex;
			pathList.push_back( node );

			node.pos = end_pos ;
			node.cell_Index=_endCellIndex;
			pathList.push_back( node);		
		}
		else
		{
			while( g_PathHeap.empty() == false  )
			{
				fwPathHeapNode currentNode;
				g_PathHeap.Top(currentNode);
				g_PathHeap.PopHead();
				visitedNodeList.insert( pair< int, fwPathHeapNode>( currentNode.kCurrentCell_Index ,currentNode) ); 

				if( currentNode.kCurrentCell_Index == endCellIndex ) //끝에서 부터 시작위치로 가는것이기에..
					break;

				//PROF_BEGIN();

				FindSmallestHeuristicCell_AddOpenList( currentNode, end_pos );

				//PROF_END();
			}

			buildPath( endCellIndex, start_pos, end_pos, pathList );
			Optimize_OmitPath( pathList ); // 직선경로는 생략시킴.
			Optimize_FineTuning( pathList ); // 생략된 경로를 미세조정함.
		}
	}



	// 시작위치와 끝위치를 넣으면 g_pathList 가 나온다.
	// 만일 시간이오래 걸리면 비동기 처리를 해야 하려나.
	// 일단은 매우 최대한 간단하고 simple 하게 유지함.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList )
	{
		pathList.clear();

		//for( size_t n=0; n< fw::GetNaviMesh().TriBuffer.size(); n++)
		//{
		//	fw::Triangle triIndex = fw::GetNaviMesh().TriBuffer[n];

		//	D3DXVECTOR3 V0 = fw::GetNaviMesh().VtxBuffer[ triIndex.index0 ].pos;
		//	D3DXVECTOR3 V1 = fw::GetNaviMesh().VtxBuffer[ triIndex.index1 ].pos;
		//	D3DXVECTOR3 V2 = fw::GetNaviMesh().VtxBuffer[ triIndex.index2 ].pos;

		//	D3DXVECTOR3 wV0,wV1,wV2;
		//	// 로컬좌표에서 이루어진 메시정보를 월드좌표로 변환
		//	D3DXVec3TransformCoord(&wV0, &V0, &fw::GetNaviMesh().LocalMat  );
		//	D3DXVec3TransformCoord(&wV1, &V1, &fw::GetNaviMesh().LocalMat);
		//	D3DXVec3TransformCoord(&wV2, &V2, &fw::GetNaviMesh().LocalMat);

		//	//picking polygon IndexList 를 가지고 있다가 최고 적은 크기 값을 유지시킨다.
		//	//float fBary2=0.0f,fBary1=0.0f,fDist=0.0f;
		//	//if( InterSectTriangle( start_pos, V0, V1, V2 ) )
		//	{
		//	//	break;
		//	}
		//}


	}

};
