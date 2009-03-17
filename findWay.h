#pragma once

#include <vector>

namespace fw
{
	/*
		viewer 에서만 쓰일값
		나중에 모조리 삭제되거나 Viewer쪽으로 빠질것임.
	*/
	extern D3DXVECTOR3 camEyePos,camLookAt;
	extern float fcamFov;

	class Mesh
	{
	public:
		struct Vertex // d3d 좌표계 기준. 화면 출력용. 디버그 용도 이외에는 쓰이지 않음.
		{
			D3DXVECTOR3 pos;
			//D3DXVECTOR3 normal;
			DWORD	Color;
		};

		// 트라이앵글에는 버텍스의 인덱스만 존재함.
		// 고로 버텍스,인덱스 버퍼는 꼭 존재해야됨.
		struct Triangle
		{
			WORD index0,index1,index2; 
		};

		std::vector< Triangle > TriBuffer;
		std::vector< Vertex > VtxBuffer;
	};

	
	/*
		return 값
		0: 정상
		1: 
	*/
	int System_Create(); // dll과 각종 자료 준비.
	void System_Shutdown(); // 로딩되었던것 모두 해제.

	/*
		desc: xml 이나 bin 데이타를 읽어들인뒤 Mesh Format 으로 컨버팅한다.
		return 값
		0: 정상.
		1: 파일을 찾을수 없다.
		2: 구버젼이다( 파싱불가 )
	*/
	int NaviMesh_LoadXml(const std::string& name ); 
	inline int NaviMesh_Load(const std::string& name ){ return NaviMesh_LoadXml(name);  }
	void NaviMesh_Release();

	const Mesh & GetNaviMesh();
	
	// 시작위치와 끝위치를 넣으면 pathList 가 나온다.
	// 만일 시간이오래 걸리면 비동기 처리를 해야 하려나.
	// 일단은 매우 최대한 간단하고 simple 하게 유지함.
	void FindWay( const D3DXVECTOR3& start_pos, const D3DXVECTOR3& end_pos, std::vector< D3DXVECTOR3> & pathList );

};
