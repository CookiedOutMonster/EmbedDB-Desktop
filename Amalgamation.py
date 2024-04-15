from collections import deque
import glob
import os
import re
import sys

'''
GLOBAL VARIABLES: DO NOT EDIT
'''

# REGEX
REGEX_INCLUDE = '\s*#include ((<[^>]+>)|("[^"]+"))'#https://stackoverflow.com/questions/1420017/regular-expression-to-match-c-include-file
REGEX_COMMENTS = '//.*?\n|/\*.*?\*/'

# Ask Ramon how he would like these dealt with
DEFINE_SEARCH_METHOD_REGEX = '#define SEARCH_METHOD [1-9]'
DEFINE_RADIX_BITS_REGEX = '#define RADIX_BITS [1-9]'
DEFINE_PRINT_ERRORS_REGEX = '#define PRINT_ERRORS'

# DIRECTORIES
ROOT_DIR = os.path.join("Implementation", "code") #change this to reflect project dir

# C-standard library 
c_stand = {
    "#include <assert.h>",
    "#include <complex.h>",
    "#include <ctype.h>",
    "#include <errno.h>",
    "#include <fenv.h>",
    "#include <float.h>",
    "#include <inttypes.h>",
    "#include <iso646.h>",
    "#include <limits.h>",
    "#include <locale.h>",
    "#include <math.h>",
    "#include <setjmp.h>",
    "#include <signal.h>",
    "#include <stdalign.h>",
    "#include <stdarg.h>",
    "#include <stdatomic.h>",
    "#include <stdbool.h>",
    "#include <stddef.h>",
    "#include <stdint.h>",
    "#include <stdio.h>",
    "#include <stdlib.h>",
    "#include <stdnoreturn.h>",
    "#include <string.h>",
    "#include <tgmath.h>",
    "#include <threads.h>",
    "#include <time.h>",
    "#include <uchar.h>",
    "#include <wchar.h>",
    "#include <wctype.h>"
}

'''
FILE PORTION
'''

def read_file(directory):
    '''
    Reads the contents of a file and returns it as a string.

    This function attempts to open a file in read-only mode and read its entire contents into a single string, which is then returned. If the function encounters an error while trying to open or read the file (such as the file not existing, lacking the necessary permissions, or the directory not being a valid file path), it will catch the exception, print an error message, and the function will return None.

    :param directory: The path to the file that needs to be read. This should be a string representing a valid file path.
    
    :return: The contents of the file as a string if the file is successfully read. Returns None if an error occurs during file reading.
    
    :raises Exception: Prints an error message with the exception detail but does not raise it further. The function handles all exceptions internally and does not propagate them.
    '''
    try:
        with open(directory, "r") as file:
            contents = file.read()
        return contents
    except Exception as e:
        sys.stderr.write(f"An error occurred opening the file: {e}\n")
        sys.exit(1) 


def save_file(source, file_name, extension):
    '''
    Writes the provided source content to a file with the specified file name and extension.

    This function creates a new file or overwrites an existing file with the given file name and extension. It writes the source content to this file. 
    The function expects the extension to be provided without a preceding dot (e.g., "txt" instead of ".txt"). 
    If an error occurs during file writing (such as permission issues or an invalid file name), the function will catch and print the exception.

    :param source: The content to be written to the file. This should be a string.
    :param file_name: The name of the file without the extension. This should be a string.
    :param extension: The extension of the file without a preceding dot. This function expects values like 'c' or 'h' for C and header files, respectively.

    :return: None. The function does not return a value but prints an error message if an exception occurs.

    :raises Exception: Catches any exceptions that occur during file writing and prints them. The function does not re-raise the exceptions.
    '''
    file = file_name + '.' + extension
    try:
        with open(file, "w") as file:
            file.write(source)
    except Exception as e:
        sys.stderr.write(f"An error occurred saving the file: {e}\n")
        sys.exit(1) 


def get_all_files_of_type(directory, file_extension):
    '''
    Searches for and returns a list of all files with a specific extension within a specified directory and its subdirectories.

    This function utilizes the `glob` module to search for files. It constructs a search pattern using the provided directory and file extension, then performs a recursive search to find all matching files. 
    If the directory does not exist or there are no files matching the given extension, the function returns an empty list.

    :param directory: The base directory from which the search should start. This should be a string representing a valid directory path.
    :param file_extension: The file extension to search for. This should be provided without a preceding dot (e.g., "txt" for text files).

    :return: A list of strings, where each string is the path to a file that matches the specified extension. If no files are found or the directory does not exist, an empty list is returned.

    Note: The function returns a relative path to each file from the current working directory.
    '''
    search_pattern = os.path.join(directory, '**', f'*.{file_extension}')
    files = glob.glob(search_pattern, recursive=True)
    return files

'''
CLEANING PORTION
'''

def retrieve_pattern_from_source(source, pattern):
    '''
    Extracts and returns all unique #include statements from a given source code string using a regular expression.

    The function employs a regular expression to identify all occurrences of #include statements in the provided source code. 
    It then formats these findings and returns them as a set, ensuring each include statement is unique in the result. 
    The regular expression used is designed to be robust and has been adapted from a solution on StackOverflow.

    :param source: The source code string from which #include statements are to be extracted.

    :return: A set of strings, each representing a unique #include statement found in the source code. If no #include statements are found, an empty set is returned.

    '''
    result = re.findall(pattern, source)
    return set([f"#include {match[0]}" for match in result])

def remove_pattern_from_source(source, pattern):
    '''
    Removes all #include statements from a C or C++ source file content.

    This function uses a regular expression to find and replace all occurrences of #include statements in the provided source code string with a space, effectively removing them. 
    The function is intended to be used with source code from .c or .h files.

    :param source: The source code string from which #include statements are to be removed. This should be the content of a .c or .h file as a string.

    :return: A string representing the source code with all #include statements removed. If no #include statements are found, the original source string is returned unchanged.
    '''
    
    return re.sub(pattern, " ", source)

def check_against_standard_library(c_stand, incoming_lib, amalg_c_stand_lib):
    '''
    Validates a set of C library headers against the Standard C Library headers and updates a set with the valid standard libraries.

    This function reads a predefined list of Standard C Library headers from a file, compares the incoming library headers against this list, and determines which of the incoming headers are part of the Standard C Library. 
    It updates the provided set with these valid standard libraries. The headers that are not part of the standard library are returned as a set.

    :param incoming_lib: A set of strings representing the library headers to be checked against the standard C library.
    :param amalg_c_stand_lib: A set that is updated with the headers from incoming_lib that are part of the standard C library.

    :return: A set of strings representing the headers from incoming_lib that are not part of the standard C library. If all incoming headers are standard, an empty set is returned.
    '''
  
    not_standard_lib = incoming_lib.difference(c_stand)
    required_standard_lib = incoming_lib - not_standard_lib

    amalg_c_stand_lib.update(required_standard_lib)
    
    # return the difference between the two sets 
    return not_standard_lib

def format_external_lib(incoming_lib):
    '''
    Transforms include statements with relative paths into just the file names.

    This function takes a set of include statements that may contain relative paths (e.g., #include "../src/EmbedDB.h") and extracts just the file names (e.g., "EmbedDB.h"). 
    It returns a new set containing these file names, ensuring that each name is unique within the set.

    :param incoming_lib: A set of strings, each representing an include statement possibly containing a relative path.

    :return: A set of strings where each string is a file name extracted from the include statements. If the incoming set is empty, the function returns an empty set.

    Note: The function assumes that the include statements are formatted with double quotes around the paths (e.g., #include "path/to/file"). 
    It does not handle angle-bracketed include statements (e.g., #include <file>), as those are assumbed to be included from c-standard libraries
    '''
    temp = set()

    # extract each dependency from incoming_lib and add it to the set 
    for lib in incoming_lib:
        path = lib.split('"')[1]  
        file = os.path.basename(path)
        temp.add(file)
    
    return temp

''' 
Dealing with FileNode
'''

def retrieve_source_set(source_dir, file_type):
    '''
    Generates a set of FileNode objects for each file of a specified type in a source directory and its subdirectories.

    This function scans a given directory and its subdirectories for files with a specified extension. It creates a FileNode object for each file found and adds it to a set, 
    ensuring that each file is represented uniquely. 

    :param source_dir: The path to the directory containing the source files. This function will also search all subdirectories.
    :param file_type: The file extension of the files to be processed (e.g., 'h' for C header files or 'c' for C source files).

    :returns: A set of FileNode objects, each representing a unique file of the specified type found in the source directory and its subdirectories.

    Note: The FileNode class should be defined elsewhere in the code, and it should have a constructor that accepts a file path as a parameter.
    '''
    file_set = set()
    dirs = get_all_files_of_type(source_dir, file_type)

    if(dirs):
        for dir in dirs:
            # create a new object for each file
            file = FileNode(dir)
            # append to the set
            file_set.add(file)
        return file_set
    else:
        sys.stderr.write(f"There was an issue opening files, no files found\n")
        sys.exit(1)


def combine_c_standard_lib(array_of_sets):
    '''
    Aggregates C standard library dependencies from multiple FileNode objects into a single set.

    This function iterates over an array of sets, where each set contains FileNode objects. 
    It extracts the C standard library dependencies (c_stand_dep) from each FileNode and combines them into a master set. 
    This master set represents all unique C standard library dependencies required by the amalgamated source file.

    :param array_of_sets: An array (or list) of sets, where each set contains FileNode objects. Each FileNode object should have a 'c_stand_dep' attribute, 
    which is a set of its C standard library dependencies.

    :return: A set containing all unique C standard library dependencies from the provided FileNode objects. If no dependencies are found, an empty set is returned.

    Note: This function assumes that each FileNode object in the sets has a 'c_stand_dep' attribute containing a set of strings, each representing a C standard library dependency.
    '''
    master_dep = set()
    
    for s3t in array_of_sets:  # love me some O(n^2)
        for file in s3t:
            dep = file.c_stand_dep
            master_dep.update(dep)

    return master_dep

'''
Directed Graph Portion 
'''

def create_directed_graph(set_of_FileNodes):
    '''
    Constructs a directed graph from a set of FileNode objects.

    This function iterates through a set of FileNode objects, using each FileNode 'file_name' attribute as a node in the graph. 
    The 'header_dep' attribute of each FileNode, which should be a set of strings representing the file names of dependencies, is used to create edges in the graph. 
    Each node in the resulting graph points to its dependencies, establishing a directed relationship.

    :param set_of_FileNodes: A set of FileNode objects. Each FileNode should have a 'file_name' attribute representing the node's name and a 'header_dep' 
    attribute representing the edges as dependencies.

    :return: A dictionary representing a directed graph. The keys are file names (nodes), and the values are sets of file names (edges) representing dependencies.

    Note: This function assumes that the 'file_name' and 'header_dep' attributes of each FileNode object are properly populated and that 'header_dep' 
    contains the names of files that each file node depends on.
    '''
    directed_graph = {}

    # Each FileNode's file_name becomes value of node, edges are the dependency set each obj contains
    for file_node in set_of_FileNodes:
        node = file_node.file_name
        edges = file_node.header_dep
        directed_graph[node] = edges

  
    return directed_graph
    
def dfs(graph, node, visited, result_stack, visiting):
    '''
    Performs a depth-first search on a graph from a specified starting node, marking visited nodes during traversal and recording the traversal order only when the last node is found to
    return a subsection of a topological result. 

    This function explores as far as possible along each branch before backtracking. It's a recursive implementation that updates the 'visited' set to keep track of 
    visited nodes and appends each node to the 'result_stack' once all its descendants have been explored. 

    :param graph: A dictionary representing the adjacency list of the graph. Keys are node identifiers, and values are sets or lists of connected nodes.
    :param node: The node from which to start the DFS.
    :param visited: A set that keeps track of which nodes have been visited to avoid revisiting them.
    :param result_stack: A list that records the order in which nodes are fully explored. Nodes are added to this stack once all their children have been visited.

    :return: None. The function updates the 'visited' set and 'result_stack' list in place.

    Note: The graph should be represented as an adjacency list, where each key is a node and its value is a collection of nodes to which it is connected. 
    The function modifies 'visited' and 'result_stack' in place, so it does not return any value.
    '''

    # add node to visited array so we do not visit it again
    visited.add(node)
    visiting.append(node)

    # for each node's decendant
    for child in graph[node]:
        # check if cylcic
        if child in visiting: 
            sys.stderr.write(f"An error occured in program depedencies, a cycle with {child} was found\n")
            sys.exit(1) 
        # only traverse unvisited nodes
        if child not in visited:
            # traverse grandchildren
            dfs(graph, child, visited, result_stack, visiting)

   
    visiting.remove(node)        
    # once at the end of dfs, add results to the stack
    result_stack.append(node)
    
def topsort(graph):
    '''
    Performs a topological sort on a directed graph.

    This function implements a topological sort algorithm, which orders the nodes in a directed graph in a linear order, respecting the dependencies represented by the edges. 
    It uses a depth-first search (DFS) to explore the graph and a stack to determine the order of nodes. 

    :param graph: A dictionary representing the adjacency list of the graph. Keys are node identifiers, and values are sets or lists of connected nodes.

    :return: A deque (double-ended queue) representing the nodes in topologically sorted order.

    Note: The graph should not contain any cycles for a valid topological sort to be possible. If the graph contains cycles, the result will not represent a valid topological ordering.
    TODO use Tarjans strongly connected component algorithm to detect if a graph has cycles. StackOverFlow also suggests that we can detect a cycle in our algorithm too
    '''

    visited = set()
    result_stack = deque() # fast stack using the collections library 
    visiting = []

    # for each node in the graph 
    for node in graph.keys():
        # if the node is not visited, search using dfs, add nodes to visited 
        if node not in visited:
            dfs(graph, node, visited, result_stack, visiting)
        

    return list(result_stack)

''' 
Creating Amalgamation
'''

def order_file_nodes_by_sorted_filenames(header_file_nodes, sorted_graph):

    '''
    Reorders a collection of FileNode objects based on a sorted list of filenames.

    This function creates a mapping from filenames to their corresponding FileNode objects and then uses a sorted list of filenames to arrange the FileNode objects in the same order. 
    The function assumes that each filename in the sorted list has a corresponding FileNode object in the input collection.

    :param header_file_nodes: A collection (e.g., list or set) of FileNode objects to be reordered. Each FileNode should have a 'file_name' attribute.
    :param sorted_graph: A list of filenames (strings) representing the desired order for the FileNode objects.

    :return: A list of FileNode objects ordered according to the order of filenames in 'sorted_graph'.

    Note: The function assumes that 'sorted_graph' contains all filenames corresponding to the FileNode objects in 'header_file_nodes' and that the 'file_name' attribute of each FileNode is unique within 
    'header_file_nodes'.
    '''

    filename_to_filenode = {}

    for FileNode in header_file_nodes:
        name = FileNode.file_name
        filename_to_filenode[name] = FileNode  

    ordered = []

    for node in sorted_graph:
        ordered.append(filename_to_filenode[node])


    return ordered

def create_amalgamation(files):

    amalgamation = ''


    for file in files: 
        if isinstance(file, FileNode):
            filename = '/************************************************************' + file.file_name + '************************************************************/\n'
            amalgamation += (filename + file.contents + '\n')
        else:
            amalgamation += (file + '\n')
   
    return amalgamation

'''
FileNode Class
'''

class FileNode():
    '''
    A class to represent a file node which encapsulates file information and dependencies.

    Attributes:
        c_stand_dep (set): A class variable that stores C standard library dependencies.
        header_dep (set): Stores header file dependencies for this specific file.
        file_dir (str): The directory path of the file.
        file_name (str): The name of the file.
        contents (str): The contents of the file after processing.

    Methods:
        __init__(self, file_dir): Initializes the FileNode with directory information, reads the file,
                                  processes its contents, and identifies dependencies.
    '''
    
    c_stand_dep = set()
    header_dep = set()
    file_dir = ''
    file_name = ''
    contents = ''

    def __init__(self, file_dir):
        '''
        Initializes the FileNode instance, reads the file, and processes its dependencies.

        The initialization process involves reading the file's content, extracting its #include statements,
        identifying local and standard library dependencies, removing include statements from the content, and
        formatting local dependencies.

        Args:
            file_dir (str): The directory path of the file to be processed.

        Steps:
            1. Assigns the file directory and extracts the file name.
            2. Reads the content of the file.
            3. Retrieves all #include statements within the file content.
            4. Identifies which of these includes are local dependencies and which are standard library dependencies.
            5. Removes all #include statements from the file content.
            6. Formats local dependencies to store only the file names.
        '''
        # assign directory
        self.file_dir = file_dir
        # retrieve file name
        self.file_name = os.path.basename(file_dir)
        # open file 
        original = read_file(file_dir)
        # retrieve included libraries
        includes = retrieve_pattern_from_source(original, REGEX_INCLUDE)
        # find local dependencies
        self.header_dep = check_against_standard_library(c_stand, includes, self.c_stand_dep)
        # remove includes and assign source
        self.contents = remove_pattern_from_source(original, REGEX_INCLUDE)
        # format local dependencies
        self.header_dep = format_external_lib(self.header_dep)

def main():
    # get source directory 
    embedDB = os.path.join('src')
    # set of objects containing source files (fileNode)
    header_file_nodes = retrieve_source_set(embedDB, 'h')
    source_file_nodes = retrieve_source_set(embedDB, 'c')
    # set of c-standard library that the amaglamation requires
    c_standard_dep = combine_c_standard_lib([header_file_nodes, source_file_nodes])
    # transform the header objects into a directed graph, nodes are file_names, edges are their dependencies
    dir_graph = create_directed_graph(header_file_nodes)
    # perform a topological sort and retrieve order for Amalgamation: not unique!
    sorted_graph = topsort(dir_graph)
    # compare sorted graph with source files
    sorted_h = order_file_nodes_by_sorted_filenames(header_file_nodes, sorted_graph)
    # combine sources
    amalg_h = create_amalgamation(c_standard_dep)+ create_amalgamation(sorted_h)
    amalg_c = '#include "./EmbedDB.h"\n' + create_amalgamation(source_file_nodes)
    # save
    save_file(amalg_h, "EmbedDB", "h")
    save_file(amalg_c, "EmbedDB", "c")
    
main()