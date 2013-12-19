#include "internetoptions.h"

InternetOptions::InternetOptions(tstring lgn, tstring pass, int invCert)
{
	login       = lgn;
	password    = pass;
	invalidCert = invCert;
	
	connectTimeout = TIMEOUT_DEFAULT;
	sendTimeout    = TIMEOUT_DEFAULT;
	receiveTimeout = TIMEOUT_DEFAULT;
}

InternetOptions::~InternetOptions()
{
}

bool InternetOptions::hasLoginInfo()
{
	return (!login.empty()) || (!password.empty());
}
