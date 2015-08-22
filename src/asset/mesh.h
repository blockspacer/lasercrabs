#pragma once
#include "types.h"

namespace VI
{

namespace Asset
{
	namespace Mesh
	{
		const int count = 12;
		const AssetID Alpha = 0;
		const AssetID city1 = 1;
		const AssetID city2 = 2;
		const AssetID city3 = 3;
		const AssetID city4_city1 = 4;
		const AssetID city4_city2 = 5;
		const AssetID city4_elevator = 6;
		const AssetID city4_shell = 7;
		const AssetID city4_shell_1 = 8;
		const AssetID city4_test = 9;
		const AssetID cube = 10;
		const AssetID skybox = 11;
	}
	const AssetID mesh_refs[4][6] =
	{
		{
			1,
		},
		{
			2,
		},
		{
			3,
		},
		{
			4,
			5,
			6,
			7,
			8,
			9,
		},
	};
}

}