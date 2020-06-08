#include "Helper.hpp"

static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

Helper::Helper()
{
	assignMIME();
}

Helper::~Helper()
{

}

std::string		Helper::findType(Request &req)
{
	std::string 	extension;

	if (req.uri.find_last_of('.') != std::string::npos)
	{
		extension = req.uri.substr(req.uri.find_last_of('.'));		
		if (MIMETypes.find(extension) != MIMETypes.end())
			return (MIMETypes[extension]);
		else
			return (MIMETypes[".bin"]);
	}
	return ("");
}

std::string		Helper::getDate()
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[BUFFER_SIZE];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, BUFFER_SIZE - 1, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

std::string		Helper::getLastModified(std::string path)
{
	char		buf[BUFFER_SIZE];
	int			ret;
	struct tm	*tm;
	struct stat	file_info;

	lstat(path.c_str(), &file_info);
	tm = localtime(&file_info.st_mtime);
	ret = strftime(buf, BUFFER_SIZE - 1, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

int				Helper::findLen(Client &client)
{
	std::string		to_convert;
	int				len;
	std::string		tmp;

	to_convert = client.rBuf;
	to_convert = to_convert.substr(0, to_convert.find("\r\n"));
	// std::cout << to_convert << ";" << std::endl;
	len = fromHexa(to_convert.c_str());
	std::cout << "l: " << len << std::endl;
	if (len != 0)
	{
		tmp = client.rBuf;
		tmp = tmp.substr(tmp.find("\r\n") + 2);
		strcpy(client.rBuf, tmp.c_str());
	}
	return (len);
}

void			Helper::fillBody(Client &client, int *len, bool *found)
{
	std::string		tmp;

	tmp = client.rBuf;
	if (tmp.size() > *len)
	{
		client.req.body += tmp.substr(0, *len);
		tmp = tmp.substr(*len + 1);
		memset(client.rBuf, 0, BUFFER_SIZE);
		strcpy(client.rBuf, tmp.c_str());
		*len = 0;
		*found = false;
	}
	else
	{
		client.req.body += tmp;
		*len -= tmp.size();
		memset(client.rBuf, 0, BUFFER_SIZE);
	}
}			

int				Helper::ft_power(int nb, int power)
{
	if (power < 0)
		return (0);
	if (power == 0)
		return (1);
	return (nb * ft_power(nb, power - 1));
}

int				Helper::fromHexa(const char *nb)
{
	char	base[17] = "0123456789abcdef";
	char	base2[17] = "0123456789ABCDEF";
	int		result = 0;
	int		i;
	int		index;

	i = 0;
	while (nb[i])
	{
		int j = 0;
		while (base[j])
		{
			if (nb[i] == base[j])
			{
				index = j;
				break ;
			}
			j++;
		}
		if (j == 16)
		{
			j = 0;
			while (base2[j])
			{
				if (nb[i] == base2[j])
				{
					index = j;
					break ;
				}
				j++;
			}
		}
		result += index * ft_power(16, (strlen(nb) - 1) - i);
		i++;
	}
	return (result);
}

std::string		Helper::decode64(const char *data)
{
	while (*data != ' ')
		data++;
	data++;
	int len = strlen(data);
	unsigned char* p = (unsigned char*)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;
    std::string str(L / 4 * 3 + pad, '\0');

    for (size_t i = 0, j = 0; i < L; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
        str[str.size() - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= B64index[p[L + 2]] << 6;
            str.push_back(n >> 8 & 0xFF);
        }
    }
    if (str.back() == 0)
    	str.pop_back();
    return (str);
}

void			Helper::negotiate(Client &client)
{
	std::map<float, std::string> 	map;
	std::string						language;
	std::string						to_parse;
	float							q;
	std::map<float, std::string>::iterator 	b;

	if (client.req.headers.find("Accept-Language") != client.req.headers.end())
	{
		to_parse = client.req.headers["Accept-Language"];
		if (to_parse.back() == '\r')
			to_parse.pop_back();
		while (to_parse != "")
		{
			language = to_parse.substr(0, to_parse.find(";"));
			std::cout << language << "!\n";
			to_parse = to_parse.substr(to_parse.find(";") + 1);
			std::cout << to_parse << "!\n";
			if (to_parse[0] == 'q')
			{
				to_parse = to_parse.substr(to_parse.find("="));
				q = atof(to_parse.c_str());
			}
			else
				q = 1;
			map[q] = language;
			if (to_parse.find(",") != std::string::npos)
				to_parse = to_parse.substr(to_parse.find(","));
			else
				to_parse = "";
		}
		b = map.begin();
		while (b != map.end())
		{
			std::cout << b->first << ":" << b->second << std::endl;
			++b;
		}
	}
}

char			**Helper::setEnv(Client &client)
{
	char								**env;
	std::map<std::string, std::string> 	envMap;

	envMap["CONTENT_LENGTH"] = std::to_string(client.req.body.size());
	envMap["CONTENT_TYPE"] = "test/file";
	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["PATH_INFO"] = client.req.uri;
	envMap["PATH_TRANSLATED"] = client.conf["path"];
	envMap["QUERY_STRING"] = client.req.uri.substr(client.req.uri.find('?') + 1);
	if (client.conf.find("exec") != client.conf.end())
		envMap["SCRIPT_NAME"] = client.conf["exec"];
	else
		envMap["SCRIPT_NAME"] = client.req.uri.substr(client.req.uri.find_last_of('/'));
	envMap["SERVER_NAME"] = "localhost";
	envMap["SERVER_PORT"] = "8080";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
	envMap["REQUEST_URI"] = client.req.uri;
	envMap["REQUEST_METHOD"] = client.req.method;
	envMap["REMOTE_ADDR"] = client.ip;

	env = (char **)malloc(sizeof(char *) * (envMap.size() + 1));
	std::map<std::string, std::string>::iterator it = envMap.begin();
	int i = 0;
	while (it != envMap.end())
	{
		env[i] = strdup((it->first + "=" + it->second).c_str());
		++i;
		++it;
	}
	env[i] = NULL;
	return (env);
}

void			Helper::freeAll(char **args, char **env)
{
	free(args[0]);
	free(args);
	int i = 0;
	while (env[i])
	{
		free(env[i]);
		++i;
	}
	free(env);
}

void			Helper::assignMIME()
{
	MIMETypes[".txt"] = "text/plain";
	MIMETypes[".bin"] = "application/octet-stream";
	MIMETypes[".jpeg"] = "image/jpeg";
	MIMETypes[".jpg"] = "image/jpeg";
	MIMETypes[".html"] = "text/html";
	MIMETypes[".htm"] = "text/html";
	MIMETypes[".png"] = "image/png";
	MIMETypes[".bmp"] = "image/bmp";
	MIMETypes[".pdf"] = "application/pdf";
	MIMETypes[".tar"] = "application/x-tar";
	MIMETypes[".json"] = "application/json";
	MIMETypes[".css"] = "text/css";
	MIMETypes[".js"] = "application/javascript";
	MIMETypes[".mp3"] = "audio/mpeg";
	MIMETypes[".avi"] = "video/x-msvideo";
}