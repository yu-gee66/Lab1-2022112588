#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstdlib>    // For system() call (Graphviz)
#include <limits>     // For numeric_limits
#include <random>     // For default_random_engine 
#include <chrono>     // For std::chrono::system_clock
#include <thread>
#include <queue>
#include <iomanip>    // For std::setw
#include <set>        // For randomWalk
#include <conio.h>    // For kbhit_、getch_

using namespace std;

// 有向图类
class Graph {
private:
    // 以邻接表表示一个有向图
    // 外层map的键是源单词，值是一个内层map。内层map中键是目标单词，值是两个单词相邻出现的次数
    // 可以简单理解为：索引为字符串的二维数组
    std::unordered_map<std::string, std::unordered_map<std::string, int>> adjList;
    // Parameters For PageRank
    const double DAMPING = 0.85;    // 阻尼系数
    const double TOLERANCE = 1e-6;  // 收敛阈值
    const int MAX_ITER = 100;       // 最大迭代次数

public:
    // 添加一条从src到dest的边，若已存在则增加计数
    void addEdge(const std::string& src, const std::string& dest) {
        adjList[src][dest]++;
    }

    // 打印有向图
    void showDirectedGraph() {
        std::vector<std::string> nodes;
        for (const auto& pair : adjList) {
            nodes.push_back(pair.first);
        }
        std::sort(nodes.begin(), nodes.end());    // 源节点按字母顺序排序

        for (const auto& node : nodes) {
            std::cout << node << " -> ";
            const auto& edges = adjList.at(node);
            std::vector<std::pair<std::string, int>> sortedEdges(edges.begin(), edges.end());
            std::sort(sortedEdges.begin(), sortedEdges.end());    // 目标节点按字母顺序排序
            for (const auto& edge : sortedEdges) {
                std::cout << edge.first << "(" << edge.second << ") ";
            }
            std::cout << std::endl;
        }
    }
    
    // 生成DOT格式并保存为文件
    void saveAsDot(const std::string& filename) {
        std::ofstream dotFile(filename + ".dot");    // 创建并打开文件
        if (!dotFile.is_open()) {
            std::cerr << "Error: Could not create DOT file." << std::endl;
            return;
        }

        dotFile << "digraph G {" << std::endl;    // 写入DOT文件头部
        for (const auto& pair : adjList) {        // 写入所有边
            const std::string& src = pair.first;
            for (const auto& edge : pair.second) {
                dotFile << "    \"" << src << "\" -> \"" << edge.first << "\" [label=\"" << edge.second << "\"];" << std::endl;
            }
        }
        dotFile << "}" << std::endl;    // 写入DOT文件尾部并关闭文件
        dotFile.close();

        // 调用Graphviz生成图像
        std::string command = "dot -Tpng " + filename + ".dot -o " + filename + ".png";
        int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "Error: Failed to generate graph image. Ensure Graphviz is installed." << std::endl;
        } else {
            std::cout << "Graph saved as " << filename << ".png" << std::endl;
        }
    }

    // 检查节点是否存在
    bool containsNode(const std::string& word) {
        return adjList.find(word) != adjList.end();
    }

    // 查询桥接词
    std::vector<std::string> findBridgeWords(const std::string& word1, const std::string& word2) {
        std::vector<std::string> bridgeWords;
        if (!containsNode(word1) || !containsNode(word2)) {
            return bridgeWords; // 返回值为空表示节点不存在
        }

        const auto& successors = adjList[word1];
        for (const auto& edge : successors) {       // 遍历word1节点的每个出边word3
            const std::string& word3 = edge.first;
            if (adjList.count(word3)) {             // 检查word3是否有出边到达word2
                const auto& successorsOfWord3 = adjList[word3];
                if (successorsOfWord3.find(word2) != successorsOfWord3.end()) {
                    bridgeWords.push_back(word3);
                }
            }
        }
        return bridgeWords;
    }

    // 随机获取一个桥接词
    std::string getRandomBridgeWord(const std::string& word1, const std::string& word2) {
        // 查询桥接词
        auto bridges = findBridgeWords(word1, word2);
        if (bridges.empty()) return "";

        // 使用时间作为随机种子
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine engine(seed);
        std::uniform_int_distribution<int> dist(0, bridges.size()-1);
        
        return bridges[dist(engine)];
    }

    // Dijkstra法计算两点最短路径
    std::pair<std::vector<std::string>, int> shortestPath(const std::string& start, const std::string& end) {
        // 初始化
        std::unordered_map<std::string, int> dist;           // 存储起点到每个节点的最短距离
        std::unordered_map<std::string, std::string> prev;   // 存储路径中每个节点的前驱节点
        std::priority_queue<std::pair<int, std::string>,     // 优先队列（最小堆）
                          std::vector<std::pair<int, std::string>>,
                          std::greater<>> pq;       
        // 初始距离设置无穷大
        for (const auto& node : adjList) {
            dist[node.first] = std::numeric_limits<int>::max();
        }
        dist[start] = 0;        // 起点到自己的距离为0
        pq.emplace(0, start);   // 将起点加入优先队列
        // Dijkstra算法
        while (!pq.empty()) {
            auto [currentDist, current] = pq.top();    // 获取当前距离最近的节点
            pq.pop();
            if (current == end) break;      // 如果到达终点，提前退出
            for (const auto& neighbor : adjList[current]) {    // 遍历所有邻居
                const std::string& next = neighbor.first;
                int weight = neighbor.second;
                int newDist = currentDist + weight;   // 计算新距离
                if (newDist < dist[next]) {     // 如果新距离比已知距离更短
                    dist[next] = newDist;       // 更新距离
                    prev[next] = current;       // 记录前驱节点
                    pq.emplace(newDist, next);  // 加入队列
                }
            }
        }
        // 不可达情况
        if (dist[end] == std::numeric_limits<int>::max()) {
            return {{}, -1};    // 返回空路径和-1表示不可达
        }
        // 回溯构建路径
        std::vector<std::string> path;
        for (std::string at = end; at != start; at = prev[at]) {
            path.push_back(at);    // 从终点回溯到起点
        }
        path.push_back(start);     // 加入起点
        std::reverse(path.begin(), path.end());   // 反转得到正序路径
        return {path, dist[end]};  // 返回路径和总距离
    }

    // 计算PageRank
    std::unordered_map<std::string, double> calculatePageRank() {
        // 初始化数据结构
        std::unordered_map<std::string, double> pr;      // 用于存储每个节点的PageRank值
        std::unordered_map<std::string, std::vector<std::string>> inEdges;   // 反向邻接表（存储指向每个节点的源节点）
        std::unordered_map<std::string, int> outDegree;       // 存储每个节点的出度
        std::vector<std::string> danglingNodes;          // 存储悬挂节点（出度为0的节点）
        // 初始化PR值和出度，识别悬挂节点
        double initialPr = 1.0 / adjList.size();    // 初始PR值（均匀分布）
        for (const auto& node : adjList) {
            pr[node.first] = initialPr;      // 设置初始PR值
            outDegree[node.first] = node.second.size();   // 计算每个节点的出度
            if (node.second.empty()) {
                danglingNodes.push_back(node.first);   // 记录悬挂节点
            }
        }
        // 构建反向邻接表
        for (const auto& src : adjList) {
            for (const auto& dest : src.second) {
                inEdges[dest.first].push_back(src.first);    // 记录指向dest的所有源节点
            }
        }
        // PageRank迭代计算
        for (int iter = 0; iter < MAX_ITER; ++iter) {
            std::unordered_map<std::string, double> newPr;   // 存储新一轮计算的PR值
            double diff = 0.0;      // 记录PR值总变化量            
            // 计算悬挂节点的总PR值
            double danglingSum = 0.0;
            for (const auto& node : danglingNodes) {
                danglingSum += pr[node];    // 累加所有悬挂节点的PR值
            }
            double danglingContribution = danglingSum / adjList.size();  // 均匀分配给所有节点
            // 计算每个节点的新PR值
            for (const auto& node : pr) {
                const std::string& u = node.first;  //当前节点
                double sum = 0.0;
                
                // 求和部分 Σ(PR(v)/L(v))
                for (const std::string& v : inEdges[u]) {
                    sum += pr[v] / outDegree[v];    // 对所有指向u的节点v，累加v的PR值除以v的出度
                }
                // 完整PR公式（包含悬挂节点的贡献）
                newPr[u] = (1.0 - DAMPING) / adjList.size() +    // 随即跳转部分 
                          DAMPING * (sum + danglingContribution);    // 链接传递部分
            }
            // 计算PR值变化量并检查收敛
            for (const auto& node : pr) {
                diff += std::abs(newPr[node.first] - pr[node.first]);  // 累加所有节点的PR变化量
            }
            pr = newPr;   // 更新PR值
            if (diff < TOLERANCE) break;    // 如果变化量小于阈值，提前终止迭代
        }
        return pr;   // 返回最终的PageRank值
    }

    // 打印PageRank结果
    void printPageRank() {
        auto pr = calculatePageRank();
        std::cout << "\nPageRank Results (d=" << DAMPING << "):\n";
        for (const auto& item : pr) {
            std::cout << std::setw(15) << item.first << ": " 
                      << std::fixed << std::setprecision(4) << item.second << std::endl;
        }
    }

    // 返回图的邻接表
    const unordered_map<string, unordered_map<string, int>>& getAdjList() const {
        return adjList;
    }

    // 随机游走功能
    void randomWalk(bool interactive = false, int stepDelayMs = 1000) {
        // interactive - 是否启用交互模式
        // stepDelayMs - 每步之间的延迟（毫秒），仅交互模式有效
        if (adjList.empty()) {
            std::cout << "Graph is empty!" << std::endl;
            return;
        }
        // 初始化随机数生成器
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine engine(seed);
        // 获取图中所有节点
        auto nodes = getNodes();
        // 创建均匀分布用于随机选择起始节点
        std::uniform_int_distribution<int> nodeDist(0, nodes.size()-1);
        std::string current = nodes[nodeDist(engine)];
        // 游走准备
        std::vector<std::string> path;   // 用于存储游走路径的节点序列
        std::set<std::pair<std::string, std::string>> visitedEdges;    // 记录已访问过的边
        bool stop = false;    // 停止标志
        path.push_back(current);  // 将起始节点加入路径
        // 打印启动信息和起始节点
        std::cout << "Random walk started. Press 's' to stop." << std::endl;
        std::cout << "Current path: " << current;
        // 主循环
        while (!stop) {
            // 检查终止条件1：无出边
            if (adjList[current].empty()) {
                std::cout << "\nStopped: Node " << current << " has no outgoing edges." << std::endl;
                break;
            }
            // 交互模式下：添加延迟
            if (interactive) {
                std::this_thread::sleep_for(std::chrono::milliseconds(stepDelayMs));
            }
            // 随机选择下一个节点
            auto& edges = adjList[current];    // 获取当前节点的所有出边
            std::vector<std::string> destinations; 
            for (const auto& edge : edges) {         // 获取所有可达的目标节点
                destinations.push_back(edge.first);  // edge.first是目标节点
            }
            // 创建均匀分布随机选择下一条边
            std::uniform_int_distribution<int> edgeDist(0, destinations.size()-1);
            std::string next = destinations[edgeDist(engine)];
            // 检查终止条件2：重复边
            auto edge = std::make_pair(current, next);   // 创建边表示（当前节点->下一节点）
            if (visitedEdges.count(edge)) {    // 检查边是否已访问过
                std::cout << "\nStopped: Repeated edge " << current << "->" << next << std::endl;
                break;
            }
            // 更新游走状态
            visitedEdges.insert(edge);  // 记录这条边已访问
            path.push_back(next);       // 将下一节点加入路径
            current = next;             // 移动到下一节点
            // 实时显示当前路径
            std::cout << " -> " << next;
            std::flush(std::cout);        // 立即刷新输出缓冲区
            // 交互模式下用户输入的检查
            if (interactive) {
                if (userWantsToStop()) {   // 检查用户是否按了's'键
                    std::cout << "\nStopped by user." << std::endl;
                    break;
                }
            }
        }
        // 保存并输出路径
        saveWalkToFile(path);
        printWalk(path);
    }

private:
    // 获取有向图中的所有节点
    std::vector<std::string> getNodes() {
        std::vector<std::string> nodes;
        for (const auto& pair : adjList) {  // 遍历邻接表中的每个键值对
            nodes.push_back(pair.first);    // 将当前节点名添加到列表中
        }
        return nodes;
    }

    // 检查用户是否想停止（仅交互模式）
    bool userWantsToStop() {
        if (_kbhit()) { // 非标准函数，Windows可用
            return _getch() == 's';    // 判断是否是's'键
        }
        // 没有按键或不是's'键
        return false;
    }

    // 保存路径到文件
    void saveWalkToFile(const std::vector<std::string>& path) {
        std::ofstream outfile("random_walk.txt");  // 打开指定文件创建输出文件流
        if (outfile.is_open()) {
            for (size_t i = 0; i < path.size(); ++i) {  // 遍历路径中的每个节点
                if (i != 0) outfile << " ";  // 如果不是第一个节点，先写入空格分隔符
                outfile << path[i];          // 写入当前节点名
            }
            outfile.close();   // 关闭文件
            std::cout << "Walk saved to random_walk.txt" << std::endl;
        } else {
            // 文件打开失败的错误处理
            std::cerr << "Unable to save walk to file." << std::endl;
        }
    }

    // 打印路径
    void printWalk(const std::vector<std::string>& path) {
        std::cout << "Random walk path (" << path.size() << " steps):\n";   // 打印路径总步数
        for (size_t i = 0; i < path.size(); ++i) {    // 遍历路径中的每个节点
            if (i != 0) std::cout << " ";   // 如果不是第一个节点，先打印空格分隔符
            std::cout << path[i];    // 打印当前节点名
        }
        std::cout << std::endl;
    }
};

// 文本预处理函数：提取文本中的英语单词
vector<string> preprocessText(const string& text) {
    vector<string> words;    // 存储最终的结果单词序列
    string currentWord;      // 存储当前正在处理的单词序列
    for (char c : text) {
        if (isalpha(c)) {    // 如果是字母字符
            currentWord += tolower(c);    // 转为小写并加入当前单词序列
        } else {                          // 如果是非字母字符（空格、标点、数字等）
            if (!currentWord.empty()) {   // 如果当前单词序列非空（说明已构建好一个单词）
                words.push_back(currentWord);    // 将当前单词序列加入结果单词序列
                currentWord.clear();      // 清空当前单词序列
            }
        }
    }
    if (!currentWord.empty()) {    // 当前单词序列中可能有剩余的单词
        words.push_back(currentWord);
    }
    return words;    // 返回结果单词序列
}

// 构建图的函数：根据单词列表构建有向图
Graph buildGraph(const vector<string>& words) {
    Graph graph;
    for (size_t i = 0; i < words.size() - 1; ++i) {
        graph.addEdge(words[i], words[i + 1]);
    }
    return graph;
}

// 读取文件的函数：读取文件并返回字符流
string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 查询桥接词函数：查询桥接词并输出查询结果
void queryBridgeWords(Graph& graph) {
    std::string word1, word2;
    std::cout << "Enter two words to find bridge words (e.g., hello world): \n";
    std::cin >> word1 >> word2;
    // 转换为小写（确保大小写不敏感）
    std::transform(word1.begin(), word1.end(), word1.begin(), ::tolower);
    std::transform(word2.begin(), word2.end(), word2.begin(), ::tolower);
    // 查询桥接词
    std::vector<std::string> bridges = graph.findBridgeWords(word1, word2);
    // 根据查询结果打印查询信息
    if (!graph.containsNode(word1) || !graph.containsNode(word2)) {
        std::cout << "No " << word1 << " or " << word2 << " in the graph!" << std::endl;
    } else if (bridges.empty()) {
        std::cout << "No bridge words from \"" << word1 << "\" to \"" << word2 << "\"!" << std::endl;
    } else {
        std::cout << "The bridge words from \"" << word1 <<"\" to \"" << word2 << "\" are: ";
        if (bridges.size() == 1) {
            std::cout << bridges[0];
        } else {
            for (size_t i = 0; i < bridges.size(); ++i) {
                if (i == bridges.size() - 1) {
                    std::cout << "and " << bridges[i];
                } else {
                    std::cout << bridges[i];
                    if (i < bridges.size() - 2) {
                        std::cout << ", ";
                    } else {
                        std::cout << " ";  // 最后一个逗号前不加空格
                    }
                }
            }
        }
        std::cout << "." << std::endl;
    }
}

// 生成文本的函数：根据桥接词生成新文本
std::string processTextWithBridges(Graph& graph, const std::string& inputText) {
    // 根据输入文本构建单词表
    std::vector<std::string> words = preprocessText(inputText);
    // 查询桥接词并插入新文本中
    std::vector<std::string> result;
    for (size_t i = 0; i < words.size(); ++i) {
        result.push_back(words[i]);    // 添加当前单词
        if (i < words.size() - 1) {    // 如果不是最后一个单词，检查当前单词是否有桥接词
            std::string bridge = graph.getRandomBridgeWord(words[i], words[i+1]);  // 随机获取一个桥接词
            if (!bridge.empty()) {
                result.push_back(bridge);    // 将桥接词加入到结果中
            }
        }
    }
    // 根据处理结果重建字符串
    std::string output;
    for (size_t i = 0; i < result.size(); ++i) {
        if (i > 0) output += " ";
        output += result[i];
    }
    return output;
}

// 找最短路径的函数：用Dijkstra算法找两点间最短路径
void findAndDisplayShortestPath(Graph& graph) {
    std::string word1, word2;
    std::cout << "Enter two words to find shortest path (e.g., hello world):\n ";
    // 使用getline读取整行输入
    std::string input;
    std::getline(std::cin, input);
    // 分割输入字符串
    std::istringstream iss(input);
    iss >> word1 >> word2;
    // 转换为小写以匹配图存储
    std::transform(word1.begin(), word1.end(), word1.begin(), ::tolower);
    std::transform(word2.begin(), word2.end(), word2.begin(), ::tolower);
    auto [path, length] = graph.shortestPath(word1, word2);
    if (length == -1) {
        std::cout << "No path exists from " << word1 << " to " << word2 << "!" << std::endl;
    } else {
        std::cout << "Shortest path (" << length << "): \n";
        for (size_t i = 0; i < path.size(); ++i) {
            if (i != 0) std::cout << " -> ";
            std::cout << path[i];
        }
        std::cout << std::endl;
    }
}

int main() {
    std::string filename;
    std::cout << "Enter the text file path: ";
    std::cin >> filename;

    // 读取文件
    std::string text = readFile(filename);
    // 获得单词列表
    std::vector<std::string> words = preprocessText(text);
    // 根据单词列表构建有向图
    Graph graph = buildGraph(words);

    // 输出有向图
    std::cout << "\nGenerated Graph:" << std::endl;
    graph.showDirectedGraph();

    // 保存为图形文件
    char choice;
    std::cout << "\nSave graph as image? (y/n): ";
    std::cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        std::string outputName;
        std::cout << "Enter output filename (without extension): ";
        std::cin >> outputName;
        graph.saveAsDot(outputName);
    }

    // 查询桥接词
    std::cout << "\n";
    char continueQuery = 'y';
    while (continueQuery == 'y' || continueQuery == 'Y') {
        // 查询桥接词
        queryBridgeWords(graph);
        // 询问是否继续
        std::cout << "\nContinue querying bridge words? (y/n): ";
        // 清除缓冲区
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        // 读取输入
        std::cin.get(continueQuery);
        // 转换为小写方便比较
        continueQuery = tolower(continueQuery);
    }
    
    // 根据桥接词生成新文本
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');    // 清除缓冲区
    std::cout << "\nEnter a new text to process with bridge words: \n";
    std::string newText;
    std::getline(std::cin, newText);
    std::string processedText = processTextWithBridges(graph, newText);
    std::cout << "Processed text with bridge words:\n" << processedText << std::endl;

    // 计算两个单词之间的最短路径
    std::cout << endl;
    findAndDisplayShortestPath(graph);

    // 计算并展示PageRank
    graph.printPageRank();
    std::cout << "\n";

    // 启动随机游走（交互模式）
    graph.randomWalk(true);   // 非交互模式为false

    // Placeholder for further graph operations
    // e.g., shortest path, connectivity, etc.

    return 0;
}