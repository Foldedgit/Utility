/*
Program: DupManager
Author: Xus

Description: The DupManager program is designed to efficiently identify and securely remove duplicate files 
from user-defined directories. The program leverages an optimized approach to search for duplicate files based
on their file size and SHA-256 hashes. The program performs the following steps:

Prompts the user to enter the directories to be processed.
Searches the directories recursively, efficiently skipping inaccessible directories and certain file types.
Maps files based on their size and filters out unique files.
Computes the SHA-256 hash for each file and maps them based on their hash.
Filters out unique files based on their hash.
Prompts the user to confirm the deletion of the duplicate files.
Moves the duplicate files to a "DeletionDuplicates" folder within the root directory of the source folder.
Creates a "paths.txt" file in the "DeletionDuplicates" folder to store the original paths of the moved files.
The program utilizes the following libraries: iostream, string, filesystem, map, vector, Windows.h, Wincrypt.h, fstream, iomanip, and sstream. 
Additionally, it utilizes the Botan library for computing SHA-256 hashes.
Note: This code has been developed for personal use and may require modifications to suit specific requirements.
*/


#include <iostream>
#include <string>
#include <filesystem>
#include <map>
#include <vector>
#define NOMINMAX
#include <Windows.h>
#include <Wincrypt.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <botan/hash.h>
#include <botan/hex.h>
#include <set>

int nfiles = 0;
std::string del = "DeletionDuplicates";

std::string toLower(const std::string& input) {
	std::string result = input;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

std::vector<std::wstring> get_input(const std::string& prompt) {
	std::cout << prompt << " : separated by commas, then press Enter:\n";
	std::wstring input;
	std::getline(std::wcin, input);

	std::wstringstream wss(input);
	std::wstring item;
	std::vector<std::wstring> result;

	while (std::getline(wss, item, L',')) {
		result.push_back(item);
	}

	return result;
}

bool confirm_action(const std::string& prompt) {

	while (true) {
		std::string input;
		std::cout << prompt << " (y/n): ";
		std::getline(std::cin, input);

		if (!input.empty()) {
			char first_char = std::tolower(input[0]);
			input[0] = first_char;
			if (input == "y") {
				return true;
			}
			else if (input == "n") {
				return false;
			}
			else {
				std::cout << "Invalid input. Please enter 'y' or 'n'." << std::endl;
			}
		}


	}
}

void print_available_root_paths() {
	DWORD buffer_size = GetLogicalDriveStrings(0, nullptr);
	if (buffer_size == 0) {
		std::cerr << "\nError: Unable to retrieve drive information." << std::endl;
		return;
	}

	std::vector<wchar_t> buffer(buffer_size);
	if (GetLogicalDriveStrings(buffer_size, buffer.data()) == 0) {
		std::cerr << "\nError: Unable to retrieve drive information." << std::endl;
		return;
	}

	std::wcout << L"\nAvailable root paths:" << std::endl;
	for (wchar_t* drive = buffer.data(); *drive; drive += wcslen(drive) + 1) {
		std::wcout << drive << std::endl;
	}
}

bool appendPathsToFile(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath, const std::filesystem::path& destFolderPath) {
	try {
		std::filesystem::path fileName("paths.txt");
		std::filesystem::path filePath = destFolderPath / fileName;

		std::ofstream file(filePath, std::ios_base::app);
		if (!file.is_open()) {
			std::cerr << "\nUnable to open file: " << filePath << std::endl;
			return false;
		}

		file << "Source: " << sourcePath << "\n";
		file << "Destination: " << destinationPath << "\n\n";
		file.close();

		std::cout << "\nPaths appended to: " << filePath << std::endl;
		return true;

	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "\nFilesystem error: " << e.what() << std::endl;
		return false;
	}
	catch (const std::exception& e) {
		std::cerr << "\nGeneral error: " << e.what() << std::endl;
		return false;
	}
}

bool moveFile(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath) {
	try {
		std::filesystem::path srcPath(sourcePath);
		std::filesystem::path destPath(destinationPath);

		if (!std::filesystem::exists(srcPath)) {
			std::cout << "Source file does not exist: " << srcPath << std::endl;
			return false;
		}

		std::filesystem::path destParentPath = destinationPath.parent_path();

		if (!std::filesystem::exists(destParentPath)) {
			std::filesystem::create_directories(destParentPath);
			std::cout << "Destination folder created: " << destParentPath << std::endl;
		}


		std::filesystem::rename(srcPath, destPath);
		std::cout << "File moved from " << srcPath << " to " << destPath << std::endl;
		return true;

	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
		return false;
	}
	catch (const std::exception& e) {
		std::cerr << "General error: " << e.what() << std::endl;
		return false;
	}
}

std::filesystem::path getdelPath(const std::filesystem::path& path) {

	std::filesystem::path inputPath(path);
	std::filesystem::path rootPath = inputPath.root_path();
	std::filesystem::path FolderPath = rootPath / del;

	return FolderPath;

}

std::wstring get_windows_directory() {
	wchar_t windowsPath[MAX_PATH];
	UINT pathLength = GetWindowsDirectoryW(windowsPath, MAX_PATH);

	if (pathLength == 0 || pathLength > MAX_PATH) {
		std::wcerr << L"\nError getting the Windows directory path." << std::endl;
		return L"";
	}

	return std::wstring(windowsPath);
}

bool is_windows_directory(const std::wstring& path) {
	std::wstring windowsDirectory = get_windows_directory();
	return !_wcsicmp(path.c_str(), windowsDirectory.c_str());
}

bool is_online_placeholder(const std::wstring& filePath) {
	DWORD fileAttributes = GetFileAttributesW(filePath.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		std::wcerr << L"\nFailed to get file attributes for: " << filePath << std::endl;
		return false;
	}

	// Check for the FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS attribute
	return (fileAttributes & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) != 0;
}

bool is_hidden(const std::filesystem::directory_entry& entry) {
	DWORD attributes = GetFileAttributesW(entry.path().wstring().c_str());
	return (attributes & FILE_ATTRIBUTE_HIDDEN) || entry.path().filename().wstring()[0] == L'.';
}

bool is_shortcut(const std::filesystem::path& path) {
	std::wstring extension = path.extension().wstring();
	return (extension == L".lnk");
}

std::vector<std::filesystem::path> get_directories_from_user() {
get_directories:
	std::vector<std::filesystem::path> directories;
	print_available_root_paths();

	for (const auto& ws : get_input("\nEnter directories to include (e.g. C:\\Folder1, D:\\, C:\\Folder2). ")) {
		// Find the index of the first non-whitespace character
		size_t first_non_whitespace = ws.find_first_not_of(L" \t\n\v\f\r");

		// Create a substring starting at the first non-whitespace character
		std::wstring path_str = ws.substr(first_non_whitespace);

		if (!std::filesystem::exists(path_str)) {
			std::wcout << L"\n\nNot exist:" << path_str << std::endl;
			goto get_directories;
		}

		directories.emplace_back(path_str);
	}

	for (const auto& path : directories) {

		try {

			std::filesystem::path delPath = getdelPath(path);
			if (std::filesystem::exists(delPath)) {
				std::cout << "Folder already exists: " << delPath << std::endl;
				continue;

			}

			if (std::filesystem::create_directory(delPath)) {
				std::cout << "New folder created: " << delPath << std::endl;

			}
			else {
				std::cout << "Failed to create folder: " << delPath << std::endl;

			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << std::endl;

		}
		catch (const std::exception& e) {
			std::cerr << "General error: " << e.what() << std::endl;

		}

	}

	return directories;
}

void search_directory(const std::filesystem::path& directory, std::map<uintmax_t, std::vector<std::string>>& file_size_to_paths_map) {
	for (auto it = std::filesystem::recursive_directory_iterator(directory, std::filesystem::directory_options::skip_permission_denied); it != std::filesystem::end(it); ++it) {
		const auto& entry = *it;

		if (!std::filesystem::exists(directory)) {
			std::wcout << L"\n\nNot exist:" << directory << std::endl;
			if (entry.is_directory()) {
				it.disable_recursion_pending();
			}
			continue;
		}


		if (is_windows_directory(entry.path())) {
			std::wcout << L"\nWindows DIR -> skipped" << std::endl;
			it.disable_recursion_pending();
			continue;

		}

		if (entry.path().filename().wstring() == L"DeletionDuplicates" || entry.path().filename().wstring() == L"RECYCLE.BIN" || is_hidden(entry) || is_online_placeholder(entry.path()) || is_shortcut(entry.path())) {
			if (entry.is_directory()) {
				it.disable_recursion_pending();
			}
			continue;
		}

		if (entry.is_directory()) {
			continue;
		}

		try {
			if (entry.is_regular_file()) {
				uintmax_t file_size = entry.file_size();
				nfiles++;
				std::cout << "\rfiles: " << nfiles << std::flush;

				// Check if the path is already in the map for the given file size
				bool path_already_exists = false;
				auto map_entry = file_size_to_paths_map.find(file_size);
				if (map_entry != file_size_to_paths_map.end()) {
					for (const auto& path : map_entry->second) {
						if (toLower(path) == toLower(entry.path().string())) {
							path_already_exists = true;
						}
					}
				}

				// If the path doesn't already exist in the map, add it
				if (!path_already_exists) {
					file_size_to_paths_map[file_size].push_back(entry.path().string());
				}
			}
		}
		catch (const std::system_error& e) {
			std::wcerr << L" \nSystem error occurred  " << L": " << e.what() << std::endl;
			continue;
		}
		catch (const std::exception& e) {
			std::wcerr << L"\nError occurred  " << L": " << e.what() << std::endl;
			continue;
		}
	}
}

std::map<uintmax_t, std::vector<std::string>> generate_file_size_to_paths_map(const std::vector<std::filesystem::path>& directories) {
	std::map<uintmax_t, std::vector<std::string>> file_size_to_paths_map;

	for (const auto& directory : directories) {
		search_directory(directory, file_size_to_paths_map);
	}
	return file_size_to_paths_map;
}

std::map<uintmax_t, std::vector<std::string>> filter_duplicates(const std::map<uintmax_t, std::vector<std::string>>& file_size_to_paths_map) {
	std::map<uintmax_t, std::vector<std::string>> duplicates_map;
	std::wcout << L"\nfilter_duplicates" << std::endl;
	nfiles = 0;
	for (const auto& [file_size, paths] : file_size_to_paths_map) {
		if (paths.size() > 1) {
			nfiles = nfiles + paths.size();
			duplicates_map[file_size] = paths;
		}
	}
	std::wcout << L"files: " << nfiles << " with same size" << std::endl;
	return duplicates_map;
}

std::string compute_sha256(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::binary);

	if (!file) {
		throw std::runtime_error("\nCould not open file: " + filepath);
	}

	std::unique_ptr<Botan::HashFunction> hash(Botan::HashFunction::create("SHA-256"));

	if (!hash) {
		throw std::runtime_error("\nFailed to create SHA-256 hash function");
	}

	// Read the file in chunks
	constexpr std::streamsize buffer_size = 4096;  // Define an appropriate buffer size, e.g., 4 KB
	std::vector<char> buffer(buffer_size);

	while (file.read(buffer.data(), buffer_size)) {
		std::streamsize bytes_read = file.gcount();
		hash->update(reinterpret_cast<const uint8_t*>(buffer.data()), bytes_read);
	}

	// Handle any remaining bytes if the file size is not a multiple of the buffer size
	std::streamsize bytes_read = file.gcount();
	if (bytes_read > 0) {
		hash->update(reinterpret_cast<const uint8_t*>(buffer.data()), bytes_read);
	}

	// Generate the final hash value
	std::string hex_output = Botan::hex_encode(hash->final());

	// Clean up
	file.close();

	return hex_output;
}

std::map<std::string, std::vector<std::string>> filter_same_sha256(const std::map<uintmax_t, std::vector<std::string>>& duplicates_map) {
	std::map<std::string, std::vector<std::string>> sha256_map;
	std::set<std::string> error_messages;
	int counter = 0;
	for (const auto& [file_size, paths] : duplicates_map) {
		for (const auto& path : paths) {
			std::string hash = "-1";
			try {
				std::string hash = compute_sha256(path);
				sha256_map[hash].push_back(path);
				counter++;
				std::cout << "\rSHA-256 Progress: " << counter << " of " << nfiles << std::flush;
			}
			catch (const std::runtime_error& e) {
				error_messages.insert(e.what());
				continue;
			}

		}
	}

	std::map<std::string, std::vector<std::string>> same_sha256_map;
	for (const auto& [hash, paths] : sha256_map) {
		if (paths.size() > 1) {
			same_sha256_map[hash] = paths;
		}
	}

	// Print unique error messages
	for (const auto& error_message : error_messages) {
		std::cerr << "\nError: " << error_message << std::endl;
	}

	return same_sha256_map;
}

int main() {
	std::cout << "\nIf you want to include your online files in the process, "
		<< "please download them first." << std::endl;
	std::cout << "\nPress Enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


	std::vector<std::filesystem::path> directories = get_directories_from_user();
	std::map<uintmax_t, std::vector<std::string>> file_size_to_paths_map = generate_file_size_to_paths_map(directories);
	std::map<uintmax_t, std::vector<std::string>> duplicates_map = filter_duplicates(file_size_to_paths_map);
	std::map<std::string, std::vector<std::string>> same_sha256_map = filter_same_sha256(duplicates_map);
	std::cout << "\n#Duplication cases: " << same_sha256_map.size() << std::endl;
	// Print or process the duplicates_map as needed
	int j = 0;
	for (const auto& [hash, paths] : same_sha256_map) {
		j++;
		std::cout << "\nCase " << j << ": " << "\nhash: " << hash << std::endl;


		int i = 0;
		for (const auto& path : paths) {
			std::cout << " \n " << i << " -> : " << path << std::endl;

			i++;
		}
		std::vector<int> delarr;

		for (const auto& wstr : get_input("\nEnter the row number to remove, ")) {
			delarr.push_back(std::stoi(wstr));
		}

		std::wcout << L"\nConfirm deletion of the following files with 'y':" << std::endl;
		for (const auto& index : delarr) {
			std::cout << "\n" << paths[index] << "\n" << std::endl;
		}

		if (confirm_action("Do you want to proceed with the action?")) {

			std::cout << "\nAction confirmed." << std::endl;
			for (const auto& index : delarr) {
				std::filesystem::path delPath = getdelPath(std::filesystem::path(paths[index]));
				std::filesystem::path filePath(paths[index]);
				std::filesystem::path fullPath = delPath / filePath.relative_path();
				moveFile(filePath, fullPath);
				appendPathsToFile(filePath, fullPath, delPath);

			}
		}
		else {
			std::cout << "\n\nAction canceled.\n\n" << std::endl;
		}




	}

	return 0;
}
