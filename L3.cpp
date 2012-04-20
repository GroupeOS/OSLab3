/****************************************************************
 * Lab3.cpp
 *
 * François Potvin Naud,
 * Michaël Guérette,
 * Mathieu Lapierre,
 * Gabriel Le Breton
 *
 * Dans le cadre du cours
 * 8INF341 - Systèmes d'exploitation
 ****************************************************************/

#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <list>
#include <stdlib.h>
#include <sys/timeb.h>
#include <string>

#define QTE_INSTRUCTIONS 2000000 //Max de cette valeur: Valeur maximum d'un int (4294967294 max pour un unsigned int)
#define MAX_FILESIZE 128 //8 fichiers dans 1024

using namespace std;

/****************************************************************
 * Variables globales et structures
 ****************************************************************/

struct file
{
    char nom[6];
    uint16_t taille;
    uint8_t premierBloc;
};

/****************************************************************
 * Headers
 ****************************************************************/

// Le disque dur
class HardDrive
{
public:
    HardDrive();
    ~HardDrive();
	void readBlock(uint8_t NumeroBlock, char*& TampLecture);
	void writeBlock (uint8_t NumeroBlock, char* TampEcriture);
	
    file readfile (uint8_t NumeroFicher);
	void writefile(uint8_t NumeroFicher, file fichier);
	
	void readFAT(uint8_t NumeroBlock, uint8_t*& TampLecture);
	void writeFAT (uint8_t NumeroBlock, uint8_t TampEcriture);
	
private:
    void Preparefile();
	void Deletefile();
	
	void* readInfile(int pos, int lenght);
	void writeInfile(int pos,int lenght, void* data);
};

class OS
{
public:
	OS();
	~OS();
	void Run();
	
private:
	void read(char nomFichier[6], int pos, int nbCaracteres, string tampLecture);
	void write(char NomFichiers [6], int position, int NombreCaracteres, char* TampEcriture);
	void deleteEOF(char NomFichiers[6], int position);
	void ManageFile();
	void AfficherStat();
	void AfficherHardDrive();
	file lireFichier(int pos);
	void ecrireFichier(int pos, file fichier);
	uint8_t lireFat(uint8_t pos);
	void ecrireFat(uint8_t pos, uint8_t val);
	void lireBloc(uint8_t pos, char* output);
	void ecrireBloc(uint8_t pos, char data[4]);
	file trouverFichier(char nomFichier[6], bool create = false);
	void creerFichierBlocVide(bool force);
	int blocSuivant(uint8_t bloc);
	
    HardDrive HD;
    int currInstruction;
    unsigned short runningtime, LastTime;
    struct timeb tp;
    vector<file> fileList;
};

/****************************************************************
 * Implementations de HardDrive
 ****************************************************************/


HardDrive::HardDrive()
{
	Preparefile();
}

HardDrive::~HardDrive()
{
	Deletefile();
}

void HardDrive::readBlock(uint8_t NumeroBlock, char*& TampLecture)
{
	//if(NumeroBlock <256)
	{
		TampLecture = (char*)readInfile(NumeroBlock*4 + 90 + 256,4);
	}
}

void HardDrive::writeBlock (uint8_t NumeroBlock, char* TampEcriture)
{
	//if(NumeroBlock <256)
	{
		writeInfile(NumeroBlock*4 + 90 + 256,4, TampEcriture);
	}
}

file HardDrive::readfile (uint8_t NumeroFicher)
{
	if(NumeroFicher < 10)
	{
		file newfile;
		char* TampLecture;
		TampLecture = (char*)readInfile(NumeroFicher*9,9);
		memcpy(&newfile,TampLecture,sizeof(file));
		return newfile;
	}
}

void HardDrive::writefile(uint8_t NumeroFicher, file fichier)
{
	if(NumeroFicher <10)
	{
		writeInfile(NumeroFicher*9,6,&fichier.nom);
		writeInfile(NumeroFicher*9+6,2,&fichier.taille);
		writeInfile(NumeroFicher*9+8,1,&fichier.premierBloc);
		
	}
	
}

void HardDrive::readFAT(uint8_t NumeroBlock, uint8_t*& TampLecture)
{
	//if(NumeroBlock <256)
	{
		TampLecture = (uint8_t*)readInfile(NumeroBlock + 90,1);
	}
}

void HardDrive::writeFAT (uint8_t NumeroBlock, uint8_t TampEcriture)
{
	//if(NumeroBlock <256)
	{
		writeInfile(NumeroBlock + 90,1, &TampEcriture);
	}
}

void HardDrive::Preparefile()
{
	char data= ' ';
	ofstream fileOut("HD.DH");
	for(int i=0;i<(90+256+1024);i++)
	{
		fileOut.write(&data, sizeof(char));
	}
	fileOut.close();
}

void HardDrive::Deletefile()
{
	remove("HD.DH");
}

void* HardDrive::readInfile(int pos, int lenght)
{
	void* buffer;
	buffer = new char[lenght];
	ifstream fileIn("HD.DH", ios::binary);
	if(fileIn != NULL)
	{
		fileIn.seekg(pos,ios_base::beg);
		fileIn.read((char*)buffer, lenght);
		fileIn.close();
	}
	return buffer;
}

void HardDrive::writeInfile(int pos,int lenght, void* data)
{
	ofstream fileOut("HD.DH",ios::out | ios::in | ios::binary);
	if(fileOut != NULL)
	{
		fileOut.seekp(pos,ios_base::beg);
		fileOut.write((char*)data, lenght);
		fileOut.close();
	}
}


/****************************************************************
 * Implementations de l'OS
 ****************************************************************/


OS::OS()
{
	currInstruction = 1;
	runningtime = 0;
	LastTime = 0;
	ftime(&tp);
	LastTime = tp.millitm;
}
OS::~OS()
{
}

void OS::Run()
{
	while(currInstruction < QTE_INSTRUCTIONS)
	{
		ManageFile();
		AfficherStat();
		currInstruction++;
	}
}

void OS::read(char nomFichier[6], int pos, int nbCaracteres, string tampLecture)
{
	file fichier = trouverFichier(nomFichier);
	if (strcmp(fichier.nom, "    ")==0)
		return;
	if (fichier.taille < pos+nbCaracteres)
		return;
	
	int bloc = fichier.premierBloc;
	for (int i = 0; i < pos/4; i++)
	{
		int temp = blocSuivant(bloc);
		if (bloc == temp)
		{
			break;
		}
		else
		{
			bloc = temp;
		}
	}
	for (int i = pos/4; i <= (pos+nbCaracteres)/4; i++)
	{
		char data[4];
		
		lireBloc(bloc, data);
		
		tampLecture.append(data, 4);
		
		int temp = blocSuivant(bloc);
		if (bloc == temp)
		{
			break;
		}
		else
		{
			bloc = temp;
		}
	}
	int firstCaractToRemove = pos - ((pos/4)*4);
	tampLecture = tampLecture.substr(firstCaractToRemove, nbCaracteres);
}

void OS::write(char NomFichiers [6], int position, int NombreCaracteres, char* TampEcriture)
{
	//cout << "WRITE " << NomFichiers << " " << position << " " << NombreCaracteres << endl;
}

void OS::deleteEOF(char NomFichiers[6], int position)
{
	//cout << "DELETE " << NomFichiers << " " << position << endl;
}

void OS::ManageFile()
{
	int pos, nbChar;
	char randChar;
	randChar = 97 + lrand48()%8;
	pos = lrand48()%MAX_FILESIZE;
	vector<file>::iterator itFile;
	
	char fileName[] = " .txt";
	fileName[0] = randChar;
	
	bool exist = false;
	itFile = fileList.begin();
	while(itFile != fileList.end() && !exist)
	{
		if(!strcmp(itFile->nom,fileName)) //Return 0 si les strings sont pareils
		{
			exist = true;
		}
		itFile++;
	}
	
	char* buffer;
	
	int action = lrand48()%3;
	if(!exist)
		action = 0;
	switch (action)
	{
			// Ecriture d'un fichier
		case 0:
		{
			nbChar = lrand48()%(MAX_FILESIZE - pos);
			buffer = new char[nbChar];
			file tmpFile;
			for(int i=0; i<6; i++)
			{
				tmpFile.nom[i] = fileName[i];
			}
			
			for(int i = 0; i<nbChar; i++)
			{
				buffer[i] = randChar;
			}
			write(fileName, pos, nbChar, buffer);
			if(!exist)
			{
				fileList.push_back(tmpFile);
			}
			break;
		}
			// Suppresion de la fin d'un fichier
		case 1:
		{
			deleteEOF(fileName, pos);
			if(pos == 0)
			{
				fileList.erase(itFile);
			}
			break;
		}
			// Suppression d'un fichier
		case 2:
		{
			deleteEOF(fileName, 0);
			fileList.erase(itFile);
			break;
		}
		default:
			break;
	}
}

void OS::AfficherStat()
{
	ftime(&tp);
	runningtime = (tp.millitm-LastTime);
	
	if (runningtime >= 5000)
	{
		//system("clear");
		
		//cout << "Affiche" <<  runningtime << endl;
		
		AfficherHardDrive();
		runningtime = 0;
		ftime(&tp);
		LastTime = tp.millitm;
	}
}

void OS::AfficherHardDrive()
{   	
	char* buffer = new char[4];
	
	system("clear");
	
	int k = 0;
	for (int i = 1; i <= 32; i++)
	{
		for (int j = 1; j <= 8; j++)
		{
			//PAS BON MERDE
			//read(fileName, pos, nbChar, buffer);
			HD.readBlock(k, buffer);
			//PAS BON!!
			cout << "0x";
			cout << setfill ('0') << setw (2);
			cout << hex << k; //hex traduit les int en hexadecimal sur 3 charactère
			cout  << " " << buffer << "  ";
			
			k++;
		}
		cout << "\n";
	}
	k = 0;
	
	vector<file> HDList;
	
	for (int i = 1; i <= 8; i++)
	{
		HDList.push_back(lireFichier(i));
	}
	
	vector<file>::iterator itFile = HDList.begin();
	
	cout << "Liste des fichiers" << endl;
	for(itFile; itFile != HDList.end(); itFile++)
	{
		int dernierBloc = itFile->premierBloc + itFile->taille;
		cout << itFile->nom << " " << itFile->premierBloc << " " << dernierBloc << " ";
	}
	cout << endl;
}

file OS::lireFichier(int pos) //pos = # de fichier (1 à 8)
{
	return HD.readfile(pos);
}

void OS::ecrireFichier(int pos, file fichier)
{
	HD.writefile(pos, fichier);
}

uint8_t OS::lireFat(uint8_t pos)
{
	uint8_t retour;
	uint8_t *ptr;
	HD.readFAT(pos, ptr);
	retour = (*ptr);
	delete ptr;
	return retour;
}

void OS::ecrireFat(uint8_t pos, uint8_t val)
{
	HD.writeFAT(pos, val);
}

void OS::lireBloc(uint8_t pos, char* output)
{
	HD.readBlock(pos, output);
}

void OS::ecrireBloc(uint8_t pos, char data[4])
{
	HD.writeBlock(pos, data);
}

file OS::trouverFichier(char nomFichier[6], bool create)
{
	
	for(int i=1; i < 10; i++)
	{
		file fichier;
		fichier = lireFichier(i);
		if(strcmp(nomFichier, fichier.nom) == 0)
		{
			return fichier;
		}
	}
	if(create)
	{
		for(int i=1; i<10; i++)
		{
			file fichier;
			fichier = lireFichier(i);
			if(strcmp("    ", fichier.nom) == 0)
			{
				strcpy(fichier.nom, nomFichier);
				fichier.taille = 0; //taille 0: aucun bloc d'alloué
				fichier.premierBloc = 0;
				
				ecrireFichier(i, fichier);
				
				return fichier;
			}
		}
	}
	file vide;
	strcpy(vide.nom,  "    ");
	vide.taille = 0;
	vide.premierBloc = 0;
	return vide;
}

void OS::creerFichierBlocVide(bool force)
{
	if (!force)
	{
		file fichier = lireFichier(0);
		if (strcmp(fichier.nom, "free.")!=0)
			return;
	}   
	
	file free;      //fichier de bloc libre
	strcpy(free.nom,"free.");
	free.taille = 1024;
	free.premierBloc = 0;
	for(int i = 0; i < 256; i++)
	{
		if (i == 255)
			ecrireFat(i, i);    //le dernier bloc d'un fichier se reconnais car il pointe sur lui-même
		else
			ecrireFat(i, i+1);
	}
}

int OS::blocSuivant(uint8_t bloc)
{
	int retour = lireFat(bloc);
	if (retour == bloc)
		return -1;
	return retour;
}

int main()
{
    OS PatOS;
    
    cout << "ON" << endl;
    for(int i = 0; i<8; i++)
    {
        PatOS.Run();
    }
    cout << "OFF" << endl;
    return 0;
}
