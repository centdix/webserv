#include "Handler.hpp"

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;
	std::string	credential;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/405.html", O_RDONLY);
		}
		else if (client.conf.find("auth") != client.conf.end())
		{
			client.res.status_code = UNAUTHORIZED;
			client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/401.html", O_RDONLY);
			if (client.req.headers.find("Authorization") != client.req.headers.end())
			{
				credential = _helper.decode64(client.req.headers["Authorization"].c_str());
				if (credential == client.conf["auth"])
					client.res.status_code = OK;
			}
		}
		if (client.res.status_code != NOTALLOWED && client.res.status_code != UNAUTHORIZED)
		{
			fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
			if (client.res.status_code == NOTFOUND)
				_helper.negotiate(client);
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == OK)
		{
			client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		}
		else if (client.res.status_code == UNAUTHORIZED)
			client.res.headers["WWW-Authenticate"] = "Basic realm=\"webserv\"";
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		errno = 0;
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE);
		if (bytes == -1)
		{
			std::cout << strerror(errno) << std::endl;
			return ;
		}
		client.wBuf[bytes] = '\0';
		if (bytes == 0)
		{
			close(client.fileFd);
			client.setToStandBy();
		}
	}
}

void	Handler::handleHead(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/405.html", O_RDONLY);
		}
		else
		{
			fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == OK)
		{
			client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		}
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
		client.setToStandBy();
	}
}

void	Handler::handlePost(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;
	int			size;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/405.html", O_RDONLY);
		}
		else if (client.conf.find("max_body") != client.conf.end()
		&& client.req.body.size() > atoi(client.conf["max_body"].c_str()))
		{
			client.res.status_code = REQTOOLARGE;
			client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/413.html", O_RDONLY);
		}
		else
		{
			if (client.conf.find("CGI") != client.conf.end()
			&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
			&& client.conf.find("exec") != client.conf.end())
				fd = open(client.conf["exec"].c_str(), O_RDONLY);
			else
				fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
		}
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
		&& client.res.status_code == OK)
		{
			client.fileFd = -1;
			execCGI(client);
			client.status = CGI;
		}
		else 
			fillStatus(client);
	}
	else if (client.status == CGI)
	{
		if (readCGIResult(client))
		{
			parseCGIResult(client);
			fillStatus(client);
		}
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		if (client.res.headers.find("Content-Length") == client.res.headers.end())
			client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		if (client.res.headers.find("Content-Type") == client.res.headers.end())
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		if (client.fileFd != -1)
		{
			size = strlen(client.wBuf);
			bytes = read(client.fileFd, client.wBuf + size, BUFFER_SIZE - size);
			client.wBuf[bytes + size] = '\0';
			if (bytes == 0)
			{
				close(client.fileFd);
				client.setToStandBy();
			}
		}
		else
		{
			size = client.file_str.size();
			bytes = write(client.fd, client.file_str.c_str(), size);
			// std::cout << "bytes write: " << bytes << std::endl;
			if (bytes < size)
				client.file_str = client.file_str.substr(bytes);
			else
			{
				client.file_str.clear();
				client.setToStandBy();
			}
		}
	}
}

void	Handler::handlePut(Client &client)
{
	int 			ret;
	std::string		path;
	struct stat	file_info;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
			client.res.status_code = NOTALLOWED;
		else
		{
			ret = open(client.conf["path"].c_str(), O_RDONLY);
			if (client.conf["isdir"] == "true")
				client.res.status_code = NOTFOUND;
			else
			{ 
				if (ret == -1)
					client.res.status_code = CREATED;
				else
				{
					client.res.status_code = NOCONTENT;
					close(ret);
				}
				client.fileFd = open(client.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
			}
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		write(client.fileFd, client.req.body.c_str(), client.req.body.size());
		client.setToStandBy();
	}
}

void	Handler::handleBadRequest(Client &client)
{
	int				bytes;
	std::string		result;
	struct stat		file_info;
	int				i;
	std::map<std::string, std::string>::const_iterator b;

	if (client.status != BODY)
	{
		client.fileFd = open("/Users/farhad/Desktop/webserv/errorPages/400.html", O_RDONLY);
		fstat(client.fileFd, &file_info);
		client.res.version = "HTTP/1.1";
		client.res.status_code = BADREQUEST;
		result = client.res.version + " " + client.res.status_code + "\n";
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		b = client.res.headers.begin();
		while (b != client.res.headers.end())
		{
			if (b->second != "")
				result += b->first + ": " + b->second + "\n";
			++b;
		}
		result += "\n";
		i = 0;
		while (result[i])
		{
			client.wBuf[i] = result[i];
			++i;	
		}
		client.wBuf[i] = '\0';
		client.status = BODY;
	}
	else
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE - 1);
		client.wBuf[bytes] = '\0';
		client.status = DONE;
	}
}