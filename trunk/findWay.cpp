#include "dxut.h"
#include "findWay.h"
#include "./tinyXml/tinyXml.h"
#include <string>

namespace fw
{
	static Mesh g_NaviMap;

	D3DXVECTOR3 camEyePos,camLookAt;
	float fcamFov;
	struct testCode
	{
		testCode()
		{
			fw::NaviMesh_LoadXml( "default.xml" );
		}
	};
	static testCode tt;	
	

	const Mesh & GetNaviMesh()
	{
		return g_NaviMap;
	}

	int NaviMesh_LoadXml(const std::string& name )
	{
		//xmlDocument 로딩하기.
		TiXmlDocument doc;
		doc.LoadFile( name.c_str() );
		g_NaviMap.TriBuffer.clear();
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
					// 실제 메쉬그룹검색완료. 여기서 버텍스와 인덱스 값을 가지고 삼각형 만들면됨.
					{ // vtx중 pos 만 추출
						TiXmlNode* pkParent = pkElem->FirstChild( "Vertex" );
						TiXmlElement* pkPosElem = pkParent ->FirstChildElement();
						while( pkPosElem )
						{
							const char* value = pkPosElem->GetText();
							float x=0.0f,y=0.0f,z=0.0f;
							sscanf_s( value, "[%f,%f,%f]", &x,&y,&z );
							fw::Mesh::Vertex vtx;
							vtx.pos = D3DXVECTOR3( x,y,z );
							
							vtx.Color = D3DXCOLOR( (rand()%256) / 256.0f, (rand()%256) / 256.0f,(rand()%256) / 256.0f,0xff );
							g_NaviMap.VtxBuffer.push_back( vtx );
							pkPosElem= pkPosElem->NextSiblingElement();
						}
					}

					//{ //normal 만 추출
					//	TiXmlNode* pkParent = pkElem->FirstChild( "VertexNormal" );
					//	TiXmlElement* pkPosElem = pkParent ->FirstChildElement();
					//	int n=0;
					//	while( pkPosElem )
					//	{
					//		const char* value = pkPosElem->GetText();
					//		float x=0.0f,y=0.0f,z=0.0f;
					//		sscanf_s( value, "[%f,%f,%f]", &x,&y,&z );
					//		fw::Mesh::Vertex & vtx = g_NaviMap.VtxBuffer[n];
					//		vtx.normal = D3DXVECTOR3( x,y,z );
					//		//g_NaviMap.VtxBuffer.push_back( vtx );
					//		pkPosElem= pkPosElem->NextSiblingElement();
					//		n++;
					//	}
					//}

					{ // TriIndex 추출.
						TiXmlNode* pkParent = pkElem->FirstChild( "TriIndex" );
						TiXmlElement* pkTriElem = pkParent ->FirstChildElement();
						while( pkTriElem )
						{
							fw::Mesh::Triangle tri;
							const char* value = pkTriElem ->GetText();
							sscanf_s( value, "[%d,%d,%d]", &tri.index0,&tri.index1,&tri.index2);

							g_NaviMap.TriBuffer.push_back( tri );
							pkTriElem = pkTriElem ->NextSiblingElement();
						}
					}
				}
				else if( strClass == "Targetcamera" )
				{
					TiXmlNode* pkTMParent= pkElem->FirstChild( "WorldTM" );
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

				}
				else if( strClass == "Targetobject" )
				{
					TiXmlNode* pkTMParent= pkElem->FirstChild( "WorldTM" );
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







};
