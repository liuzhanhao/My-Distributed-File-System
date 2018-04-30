#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

void split(string original_name, int n) {
	std::ifstream is(original_name, std::ifstream::binary);
	if (is) {
		// get length of file:
	    is.seekg (0, is.end);
	    int length = is.tellg();
	    is.seekg (0);
	    cout << "file length: " << length << endl;

	    int chunk_size = length / n;
	    int last_chunk_size = chunk_size + length % n;
	    string raid5_buf(last_chunk_size, 0);

	    // write file chunk
	    for (int i = 0; i < n; i++) {
	    	if (i == n - 1)
	    		chunk_size = last_chunk_size;
	    	
	    	string buf;
	    	buf.resize(chunk_size);
	    	is.read(&buf[0], chunk_size);

	    	// raid 5
	    	for (int j = 0; j < chunk_size; j++)
	    		raid5_buf[j] ^= buf[j];

	    	string file_name = original_name + "-part" + std::to_string(i);
	    	std::ofstream os(file_name, std::ofstream::binary);
	    	os.write(&buf[0], chunk_size);
	    	os.close();
	    }

	    // write raid-5 chunk
	    string file_name = original_name + "-raid5";
    	std::ofstream os(file_name, std::ofstream::binary);
    	os.write(&raid5_buf[0], last_chunk_size);
    	os.close();

    	/* 
    		write file_size into a file, this file should be stored in the raid-5 node
    		because we need file_size to calculate the broke_chunk's size when 
    		recovering using raid-5
    	*/
    	file_name = original_name + "-size";
    	std::ofstream os_size(file_name);
    	os_size << length;
    	os_size.close();

	    if (is)
	      cout << "all characters read successfully." << endl;
	    else
	      cout << "error: only " << is.gcount() << " could be read" << endl;
	    is.close();
	}
}

void merge(string original_name, string new_name, int n) {
	std::ofstream os(new_name, std::ofstream::binary);
	if (os) {
	    // read file chunk
	    for (int i = 0; i < n; i++) {
	    	string file_name = original_name + "-part" + std::to_string(i);
	    	std::ifstream is(file_name, std::ifstream::binary);

	    	// get length of file:
		    is.seekg (0, is.end);
		    int length = is.tellg();
		    is.seekg (0);
		    cout << "file length: " << length << endl;
	    	
	    	string buf;
	    	buf.resize(length);
	    	is.read(&buf[0], length);

	    	os.write(&buf[0], length);
	    	is.close();
	    }
	    os.close();
	}
}

// broke_chunk: the index of the chunk which is broken, which will be skipped in recover
void recover(string original_name, string new_name, int n, int broke_chunk) {
	std::ofstream os(new_name, std::ofstream::binary);

	// read raid-5 file
	string raid5_file = original_name + "-raid5";
	std::ifstream is_raid5(raid5_file, std::ifstream::binary);
	// get length of file:
    is_raid5.seekg (0, is_raid5.end);
    int last_chunk_size = is_raid5.tellg();
    is_raid5.seekg (0);
    cout << "last_chunk_size: " << last_chunk_size << endl;
    // read raid-5 file into buf
    string raid5_buf;
	raid5_buf.resize(last_chunk_size);
	is_raid5.read(&raid5_buf[0], last_chunk_size);
	is_raid5.close();

	// find the size of the broke_chunk
	string size_file = original_name + "-size";
	std::ifstream is_size(size_file);
	// get length of file:
	string line;
    std::getline(is_size, line);
    int file_size = stoi(line);
	is_size.close();
	cout << "file_size: " << file_size << endl;
	int chunk_size = file_size / n;

	if (os) {
	    // read file chunk for 0 to broke_chunk - 1
	    for (int i = 0; i < broke_chunk; i++) {
	    	string file_name = original_name + "-part" + std::to_string(i);
	    	std::ifstream is(file_name, std::ifstream::binary);

	    	// get length of file:
		    is.seekg (0, is.end);
		    int length = is.tellg();
		    is.seekg (0);
		    cout << file_name << " length: " << length << endl;
	    	
	    	string buf;
	    	buf.resize(length);
	    	is.read(&buf[0], length);

	    	// raid 5
	    	for (int j = 0; j < length; j++)
	    		raid5_buf[j] ^= buf[j];

	    	os.write(&buf[0], length);
	    	is.close();
	    }
	    // vector of string for broke_chunk + 1 to n-1
	    std::vector<string> v;
	    for (int i = broke_chunk + 1; i < n; i++) {
	    	string file_name = original_name + "-part" + std::to_string(i);
	    	std::ifstream is(file_name, std::ifstream::binary);

	    	// get length of file:
		    is.seekg (0, is.end);
		    int length = is.tellg();
		    is.seekg (0);
		    cout << file_name << " length: " << length << endl;
	    	
	    	string buf;
	    	buf.resize(length);
	    	is.read(&buf[0], length);
	    	v.push_back(buf);

	    	// raid 5
	    	for (int j = 0; j < length; j++)
	    		raid5_buf[j] ^= buf[j];

	    	is.close();
	    }
	    // write broke chunk
	    // check if broke_chunk's size equals raid5_buf's size
	    if (broke_chunk != n - 1){
	    	raid5_buf = raid5_buf.substr(0, chunk_size);
	    	os.write(&raid5_buf[0], chunk_size);
	    }
	    else {
	    	os.write(&raid5_buf[0], last_chunk_size);
	    }
	    // write the remaining chunks (for broke_chunk+1 to n-1)
	    for (int i = broke_chunk + 1; i < n; i++) {
	    	if (i < n - 1) {
	    		os.write(&v[i-broke_chunk-1][0], chunk_size);
	    	}
	    	else {
	    		os.write(&v[i-broke_chunk-1][0], last_chunk_size);
	    	}
	    }
	    os.close();
	}
}

int main(int argc, char** argv) {
	string original_name = argv[2];
	int n = 3;
	if (strcmp(argv[1], "split") == 0) {
		cout << "spliting " << original_name << endl;
		split(original_name, n);
	}
	else if (strcmp(argv[1], "merge") == 0) {
		cout << "merging " << original_name << endl;
		merge(original_name, "merge", n);
	}
	else if (strcmp(argv[1], "recover") == 0) {
		cout << "recovering " << original_name << endl;
		recover(original_name, "recover", n, 0);
	}
	else
		cout << "invalid command" << endl;
}