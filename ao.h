class AO {
		
public:;
	
		AO();	
	int	Init();
	int	Send(const char *);
	void	Center();
	void	Set(int x, int y);
private:
	int	ao_fd;
	int	xpos;
	int	ypos;
};


