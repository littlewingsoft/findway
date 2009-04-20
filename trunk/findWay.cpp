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
4. Heap �����
heap�� End��ǥ�� ���� �Ųٷ� start������ �� ������ ����Ʈ �Դϴ�.
end cell�� ������, start cell�� ��ǥ������ �����մϴ�.
EndCell�� ���� ������ ������ ���մϴ�. �� ���� ������ ����� ���� �� ���� ��� �մϴ�. ������ Heuristic + ArrivalCost �Դϴ�. 
 
���⼭ Heristic��
deltax = (��ǥ��.x - ����������.x)
deltay = (��ǥ��.y - ����������.y)
deltaz = (��ǥ��.z - ����������.z)
max(max(deltax, deltay), deltaz)
���Դϴ�.
 
�̿��� ���� ����� �� ���� �������� �Ǹ� ���� ������� �ٽ� �̿��� ���� �� �մϴ�.
�̸� �ݺ� �ϸ� start ������ ���� �̵� �˴ϴ�.
 
5. Path �����
�տ��� ���� heap���� ���� start�� ���� end������ mid��ǥ�� cell�� ���������� ������ �Ӵϴ�.
 
6. �߰� �۾�
�ﰢ���� �������� Cell���� �̵��� �ٴϰ� �Ǹ� ������(������) Actoró�� ���̹Ƿ�, �տ��� ������ Path�� ���� line�׽�Ʈ�� �Ͽ� ���� ��θ� ���� �̸� �̵� ��η� ��� �մϴ�.
[��ó] �׺���̼� �޽� + A* (Navigation Mesh + AStar)|�ۼ��� ������

��ǥ��(��������)->  ������(������)

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

	// �� ������� �ƴϸ� node ó���� �Ǹ� �����Ұ͵�.
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
		{ // vtx�� pos �� ����
			TiXmlElement* pkFirstElem = pkElem->FirstChildElement( "WIRECOLOR" );
			const char* value = pkFirstElem->GetText();
			int r=0,g=0,b=0;
			sscanf_s( value, "%d,%d,%d", &r,&g,&b );
			wireClr = D3DXCOLOR( r/255.0f,g/255.0f,b/255.0f, 1.0f );
		}


		// ���� �޽��׷�˻��Ϸ�. ���⼭ ���ؽ��� �ε��� ���� ������ �ﰢ�� ������.
		{ // vtx�� pos �� ����
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

		{ // TriIndex ����.
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
		////������ �̿��� �븻�� ���Ѵ�.
		D3DXVECTOR3 v0 = p1 - p0;
		D3DXVECTOR3 v1 = p2 - p0;
		D3DXVECTOR3 Nomal;
		D3DXVec3Cross(&Nomal, &v0, &v1);
		D3DXVec3Normalize(&Nomal, &Nomal);

		////3���� 3���� �븻�� �����Ѵ�.
		D3DXPLANE Plane1;
		D3DXVECTOR3 n1;
		D3DXPLANE Plane2;
		D3DXVECTOR3 n2;
		D3DXPLANE Plane3;
		D3DXVECTOR3 n3;

		////������ ������ ������ ���Ѵ�.
		n1 = (p0 + p2) / 2.0f + Nomal;
		n2 = (p0 + p1) / 2.0f + Nomal;
		n3 = (p2 + p1) / 2.0f + Nomal;


		////������ ���� �븻������ ����� �����.
		D3DXPlaneFromPoints(&Plane1, &p2, &p0, &n1);
		D3DXPlaneFromPoints(&Plane2, &p0, &p1, &n2);
		D3DXPlaneFromPoints(&Plane3, &p1, &p2, &n3);

		float d1, d2, d3;

		////����� �����Ŀ� �����Ѵ�.

		//// ax + by + cz +d < 0 ���� �ۿ� �ִ�.

		//// m.x m.y m.z �� ���� ��ǥ�Դϴ�.
		d1 = m.x * Plane1.a + m.y * Plane1.b + m.z * Plane1.c + Plane1.d;
		d2 = m.x * Plane2.a + m.y * Plane2.b + m.z * Plane2.c + Plane2.d;
		d3 = m.x * Plane3.a + m.y * Plane3.b + m.z * Plane3.c + Plane3.d;

		////d1 d2 d3�� ���δ� 0���� ũ�� ��鳻�ο� �ִ�.
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
	static std::map<int, fwPathNode> closeList; //�̹� ó�� ������.

	/*
		 ���� ���� ��� �̿����� ã�´�. 
		 if ( ���¸���Ʈ.ã��(�̿���) == �ִ� )
		 { ������ �׳� �Ѿ. }
		 else
		 { �ε����� ���¸���Ʈ�� �ִ´�. }	
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
			if( neighborIndex == -1 || cnt != 0 )  // �̿��� �־����.
				continue;

			const fwNaviCell& neighborCell = kMesh.CellBuffer[neighborIndex];
			// �ѹ��� �߰��������� ��Ҹ� ���¸���Ʈ�� �־����.


			// �θ�κ��� ����ġ�����ǰŸ�. 
			// ���缿�� �θ� �ǰ� �̿����� �̶� ���缿�̵ȴ�.
			///float G_costFromParent = 0.0f;
			float G_costFromStart =  topNode.costFromStart+ currCell.arrivalCost[ indexCnt ] ;//���缿���� �����Ȱ� + �̿��� ���ڽ�Ʈ.
			float H_costToGoal = ComputeHeuristic( neighborCell.center , endPos );
			fw::fwPathNode newNode( cellIndex , neighborIndex, G_costFromStart ,H_costToGoal );

			g_PathHeap.AddPathNode( newNode ); // ���⼭ �߰��Ȱ�üũ�� �ѵڿ� �߰��� �Ȼ��¸� ���� �����Ѵ�.
		}


		return -1;
	}




/**



���
������� : ���ɼ��� �ִ� ������ ������ ���.
������� : ���ɼ��� ���� ������ ������ ���. ���� �ٽô� �� �ʿ䰡 ����.


����
������� : �켱���� ť (F=G+H ���� �������� ���� �켱����)

��������� �켱���� ť�� �� ��Ȳ�� �ƴϰų�, �ٸ� �ڷᱸ���� �� �ʿ伺�� ������ ���� �Ʒ� �˰��򿡼� �������.�� �� ������ ���� �����ϸ� �ȴ�

�������.�� - ������� �� F=G+H ���� ���� ���� ���� ������ �ȴ�

������� : ��� �ڷᱸ�� ������ ��(�ӵ��� ���ؼ���� map�̰���)

 
1) �������.�߰�(��������)
2) �ݺ�
{
  1) ���缿 = �������.��()
  2) �������.Ǫ��(���缿)
  3) foreach(�̿��� in ���缿 ����)
  {
    1) if(�̿���.��������() && not �������.����(�̿���))

      1) if(not �������.����(�̿���))

        1) �������.�߰�(�̿���)
        2) �̿���.�θ� = ���缿
        3) �̿���.F�����() | ��� >> F = G(�̿���.�θ�.F���) + H(GOAL���� �Ÿ�) | �̿���.�θ� �� ���������̹Ƿ� G �� ���缿.F������� �ϸ� ��

      2) else // if(�������.����(�̿���))

        1) �̿���.G�����() | ��� >> ������.F���(�̿���.�θ� �� �����ȵǾ��������Ƿ� �̿���.�θ�.F������� �ϸ� �ȵ�)
        2) ���� ����� �� ������

          1) �̿���.�θ� = ���缿
          2) �̿���.F�����() | ��� >> F = G(�̿���.�θ�.F���) + H(GOAL���� �Ÿ�) | �̿���.�θ� �� ���������̹Ƿ� G �� ���缿.F������� �ϸ� ��

  }

    2) ��ǥ������ ������Ͽ� �߰��ߴٸ�, loop ��������
    3) ��������� ����ٸ�, ã�µ� ���������Ƿ� loop ��������
}

3) �������ϱ� | ��� >> ��ǥ�������� ���� �� �θ������� ���󰡴�, ù������ ���� ������ ���ÿ� �����϶�.
4) ����󰡱� | ��� >> ���ÿ��� �ϳ��� pop() �ϸ� �ȴ�.
	*/


	
	// �̹� �ܺο����� ������ ���� �Ѱ��ְ� �ؾ� �ɵ�. ��ŷ���� �ﰢ��(��) �ε����� ã�Ƽ� �Ѱ����. 
	// �ϴ� pathList �� �ִܰŸ� ����ȭ�� �����ʴ´�.
	void FindWay( const int endCellIndex, const D3DXVECTOR3& end_pos, const int startCellIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & pathList )
	{
		pathList.clear();
		g_PathHeap.clear();
		closeList.clear();
	
		// ���ο� �̿����� üũ�Ҷ� ���⿡ �ִ��� üũ�غ��� �ִٸ� �ǳʶ�.
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

				closeList.insert( pair< int, fwPathNode>( currentNode.kCurrentCell_Index ,currentNode) ); // �̸� �־ ����.

				FindSmallestHeuristicCell_AddOpenList( currentNode.kCurrentCell_Index, end_pos );
				//
			}

			// ���������� �������� ��Ҵٸ� �װ��� top �� �ɰ��̴�.
			// �װ��� �θ��带 �������� ã�ư���.
			fwPathNode node;
			g_PathHeap.Top( node );
			map<int,fwPathNode>::iterator it = closeList.begin();
			if( closeList.count( node.kParentCell_Index ) != 0 )
			{
				fwPathNode tmpnode = closeList[ node.kParentCell_Index ];
				while( tmpnode.kParentCell_Index != - 1 )
				{//�θ��尡 ���۳��� ���������� ��� ã��.���۳�常 �θ� -1 �� ���õ�����.
					
					D3DXVECTOR3& pos = fw::GetNaviMesh().CellBuffer[ tmpnode.kCurrentCell_Index].center;
					pathList.push_back( pos );
					tmpnode = closeList[ tmpnode.kParentCell_Index];
					
				}

			}
			//g_PathHeap.top 
			// ������ ���� ��� ã������ end_pos �� ������ġ��.
		pathList.push_back( start_pos );
			
		}

	}


	void deprecated_FindWay( const int endCellIndex, const D3DXVECTOR3& end_pos, 
				const int startCellIndex, const D3DXVECTOR3& start_pos, std::vector< D3DXVECTOR3> & pathList )
	{

	}


	// ������ġ�� ����ġ�� ������ pathList �� ���´�.
	// ���� �ð��̿��� �ɸ��� �񵿱� ó���� �ؾ� �Ϸ���.
	// �ϴ��� �ſ� �ִ��� �����ϰ� simple �ϰ� ������.
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
		//	// ������ǥ���� �̷���� �޽������� ������ǥ�� ��ȯ
		//	D3DXVec3TransformCoord(&wV0, &V0, &fw::GetNaviMesh().LocalMat  );
		//	D3DXVec3TransformCoord(&wV1, &V1, &fw::GetNaviMesh().LocalMat);
		//	D3DXVec3TransformCoord(&wV2, &V2, &fw::GetNaviMesh().LocalMat);

		//	//picking polygon IndexList �� ������ �ִٰ� �ְ� ���� ũ�� ���� ������Ų��.
		//	//float fBary2=0.0f,fBary1=0.0f,fDist=0.0f;
		//	//if( InterSectTriangle( start_pos, V0, V1, V2 ) )
		//	{
		//	//	break;
		//	}
		//}


	}

};
