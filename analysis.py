import os.path

strFormat = '%-14s%-14s%-14s%-14s%-14s%-14s%-14s%-14s\n'
strFormat2 = '%-14s%-14s%-14s%-14s%-14s%-14s%-14s%-14s%-14s\n'

Strategies = ['cho', 'single', 'max', 'random']

for strategy in Strategies:
	popt = []
	with open('./result/' + strategy + '/config.json') as conFile:
		for line in conFile:
			if "popt" in line:
				popt.append(int(line.split(':')[1].rstrip().replace('"','').strip(',')))
	count = 1
	filename = './result/' + strategy + '/' + strategy + '-'
	for i in range(len(popt)):
		if i == 0:
			newfile = open(filename + str(i) + '.txt', 'w+')
		else:
			newfile = open(filename + str(i) + '.txt', 'a')
		for j in range(popt[i]):
			cnt = 0
			mode = 0
			correction = [0, 0, 0, 0]
			if os.path.isfile(filename + 'thread-' + str(count) + '.txt'):
				with open(filename + 'thread-' + str(count) + '.txt') as resFile:
					for line in resFile:
						if 'mode' in line:
							if j == 0:
								newfile.write(line)
								mode = int(line.split(' ')[1])
								if(mode == 1):
									correction[1] = 70000
								if(mode == 2):
									correction[2] = 30000
						elif 'tid' in line:
							if j == 0:
								newline = line.split('\t')
								newfile.write(strFormat2 %(newline[0], newline[1], newline[2], newline[3], newline[4], newline[5], newline[6], newline[7], newline[8])) 
						else:
							newline = line.split('\t')
							if(mode == 1 or mode == 2):
								#newline[3] = str(int(newline[3]) - correction[mode])
								#newline[4] = str(int(newline[4]) - correction[mode])
								#newline[8] = str(int(newline[8]) - correction[mode])
								newfile.write(strFormat2 %(newline[0], newline[1], newline[2], newline[3], newline[4], newline[5], newline[6], newline[7], newline[8])) 
							else:
								newfile.write(strFormat2 %(newline[0], newline[1], newline[2], newline[3], newline[4], newline[5], newline[6], newline[7], newline[8])) 
				count += 1
		newfile.close()
