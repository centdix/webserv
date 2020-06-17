#ifndef HELPER_HPP
#define HELPER_HPP

#include <sys/stat.h>
#include <sys/time.h>

#include "statusCode.h"
#include "Client.hpp"

class Helper
{
	public:
		std::map<std::string, std::string> MIMETypes;

		Helper();
		~Helper();

		std::string		getDate();
		std::string		getLastModified(std::string path);
		std::string		findType(Request &req);
		void			getErrorPage(Client &client);
		int				findLen(Client &client);
		void			fillBody(Client &client);
		char			**setEnv(Client &client);
		std::string		decode64(const char *data);
		void			parseAcceptLanguage(Client &client, std::multimap<std::string, std::string> &map);
		void			parseAcceptCharsets(Client &client, std::multimap<std::string, std::string> &map);

		int				ft_power(int nb, int power);
		int				fromHexa(const char *nb);
		void			freeAll(char **args, char **env);
		void			assignMIME();

		int				getStatusCode(Client &client);
		int				GETStatus(Client &client);
		int				POSTStatus(Client &client);
		int				PUTStatus(Client &client);

};

#endif