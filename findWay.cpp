#include "dxut.h"

#include <algorithm>
#include <string>
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

목표점(루프시작)->  시작점(루프끝)

*/
namespace fw
{
	class exc : public exception 
	{
	public:
		exc()
		{
			//assert(0);
		}
	};

	// 곧 지우던가 아니면 node 처리가 되면 정리할것들.
	D3DXVECTOR3 camEyePos,camLookAt;
	float fcamFov;
	float fNear, fFar;

	static std::map<string,fwMesh> g_MeshList;

	struct testCode
	{
		testCode()
		{
			fw::NaviMesh_Load( "tedefault.xml" );
			//fw::NaviMesh_Load( "default.xml" );
			fw::AddMesh_FromXml( "agent.xml" );
			fw::AddMesh_FromXml( "point.xml" );
			fw::AddMesh_FromXml( "Sphere01.xml" );
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
			throw exc();
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





	fwMesh& GetNaviMesh()
	{
		const std::string szCONVENTION_NAVI = "navi_";
		map< string, fwMesh >::iterator it = g_MeshList.begin();
		
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

		throw exc();
	}


	inline fwMesh& GetMesh(const string&name )
	{
		std::string strName = name;
		std::transform( strName.begin(), strName.end(), strName.begin(), tolower );
		
		if( g_MeshList.count( strName ) == 0 )
		{
			throw exc() ;

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
	static std::map<int, fwPathNode> closeList; //이미 처리 끝난것.

	/*
		 현재 쎌의 모든 이웃셀을 찾는다. 
		 if ( 오픈리스트.찾기(이웃셀) == 있다 )
		 { 있으면 그냥 넘어가. }
		 else
		 { 인덱스를 오픈리스트에 넣는다. }	
	*/
	int FindSmallestHeuristicCell_AddOpenList( int cellIndex, const D3DXVECTOR3& endPos )
	{
		const fwMesh& kMesh = GetMesh("navi_ground");
		fwPathNode topNode ;
		g_PathHeap.Top( topNode );			
		g_PathHeap.PopHead();
		const fwNaviCell& currCell= kMesh.CellBuffer[ cellIndex ];

		for( int indexCnt= 0; indexCnt<3; indexCnt++ )
		{
			int neighborIndex = currCell.NeighborTri[ indexCnt ];
			int cnt = closeList.count( neighborIndex );
			if( neighborIndex == -1 || cnt != 0 )  // 이웃이 있어야함.
				continue;

			const fwNaviCell& neighborCell = kMesh.CellBuffer[neighborIndex];
			// 한번도 추가된적없는 요소를 오픈리스트에 넣어야함.


			// 부모로부터 내위치까지의거리. 
			// 현재셀이 부모가 되고 이웃셀은 이때 현재셀이된다.
			///float G_costFromParent = 0.0f;
			float G_costFromStart =  topNode.costFromStart+ currCell.arrivalCost[ indexCnt ] ;//현재셀까지 누적된값 + 이웃셀 의코스트.
			float H_costToGoal = ComputeHeuristic( neighborCell.center , endPos );
			fw::fwPathNode newNode( cellIndex , neighborIndex, G_costFromStart ,H_costToGoal );

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


	
	// 이미 외부에서는 반직선 값만 넘겨주게 해야 될듯. 피킹으로 삼각형(셀) 인덱스를 찾아서 넘겨줬다. 
	// 일단 pathList 는 최단거리 최적화는 하지않는다.
	void FindWay( const int endCellIndex, const D3DXVECTOR3& end_pos, const int startCellIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & pathList )
	{
		pathList.clear();
		g_PathHeap.clear();
		closeList.clear();
	
		// 새로운 이웃셀을 체크할때 여기에 있는지 체크해본뒤 있다면 건너뜀.
		int  currCell = startCellIndex;
		fwPathNode node( -1, startCellIndex, 0, ComputeHeuristic( start_pos, end_pos ) );
		g_PathHeap.AddPathNode( node );
		pathList.push_back( end_pos );				
		if( currCell == endCellIndex  )
		{
			pathList.push_back( start_pos );
		}
		else
		{
			while( g_PathHeap.empty() == false )
			{
				fwPathNode currentNode;
				g_PathHeap.Top(currentNode);
				if( currentNode.kCurrentCell_Index == endCellIndex )
					break;

				closeList.insert( pair< int, fwPathNode>( currentNode.kCurrentCell_Index ,currentNode) ); // 미리 넣어도 무방.

				FindSmallestHeuristicCell_AddOpenList( currentNode.kCurrentCell_Index, end_pos );
				//
			}

			// 최종적으로 목적지에 닿았다면 그것이 top 이 될것이다.
			// 그것의 부모노드를 차곡차곡 찾아가자.
			fwPathNode node;
			g_PathHeap.Top( node );
			map<int,fwPathNode>::iterator it = closeList.begin();
			if( closeList.count( node.kParentCell_Index ) != 0 )
			{
				fwPathNode tmpnode = closeList[ node.kParentCell_Index ];
				while( tmpnode.kParentCell_Index != - 1 )
				{//부모노드가 시작노드와 같을때까지 계속 찾기.시작노드만 부모가 -1 로 셋팅되있음.
					
					D3DXVECTOR3& pos = fw::GetNaviMesh().CellBuffer[ tmpnode.kCurrentCell_Index].center;
					pathList.push_back( pos );
					tmpnode = closeList[ tmpnode.kParentCell_Index];
					
				}

			}
			//g_PathHeap.top 
			// 보간될 길을 모두 찾았으면 end_pos 가 최종위치임.
		pathList.push_back( start_pos );
			
		}

	}


	void deprecated_FindWay( const int endCellIndex, const D3DXVECTOR3& end_pos, 
				const int startCellIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & pathList )
	{

	}


	// 시작위치와 끝위치를 넣으면 pathList 가 나온다.
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
