#pragma once

namespace show_off
{
	struct RunParams
	{
		const int CHAR	      = 0;
		const int UCHAR	      = 0;
		const int SHORT	      = 0;
		const int USHORT      = 0;
		const int INT	      = 0;
		const int UINT	      = 0;
		const int LL	      = 0;
		const int ULL	      = 0;
		const int FLOAT	      = 0;
		const int DOUBLE      = 0;
		const int STRING      = 0;
		const int COMPLEX_I32 = 0;
		const int COMPLEX_LL  = 0;
		const int COMPLEX_FP  = 0;
		const int COMPLEX_STR = 0;
	};

	void showOff(RunParams params);

	void validate(RunParams params);
};