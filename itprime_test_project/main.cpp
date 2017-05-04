#include <string>
#include <istream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>

class Test;

class WordConverter
{
  friend Test;
private:
  static const int _invalidIsWordUsed = -1;
  static const int _maxWordDistance = 1;

  std::vector<std::string> _dict;
  std::vector<int> _isWordUsed;
  size_t _sourceWordIndex;
  size_t _targetWordIndex;

  size_t getWordIndex(std::string word)
  {
    for (size_t i = 0; i < _dict.size(); ++i)
      if (_dict[i] == word)
        return i;
    throw std::runtime_error("Dictionary should contain source and target words");
  }

  void fillDictionary(std::istream &dictStream, size_t wordLen)
  {
    while (!dictStream.eof())
    {
      std::string word;
      std::getline(dictStream, word);
      if (word.size() != wordLen)
        continue;
      _dict.push_back(word);
    }
    std::sort(_dict.begin(), _dict.end());
  }

  unsigned int wordDistance(const std::string &w1, const std::string &w2) const 
  {
    assert(w1.size() == w2.size());
    unsigned int dist = 0;
    for (size_t i = 0; i < w1.size(); ++i)
    {
      dist += ((w1[i] - w2[i]) != 0);
      //TODO: нет смысла считать если расстояние уже равно 1
    }
    return dist;
  }

  std::vector<size_t> getSimilarWords(const std::vector<size_t> & wordIndexes) const
  {
    std::vector<size_t> similarWordIndexes;
    std::for_each (wordIndexes.begin(), wordIndexes.end(), [&](size_t wordIndex) 
    {
      const std::string & currentWord = _dict[wordIndex];
      //TODO: можно ускорить поиск за счет переноса уже учтенных слов в конец массива
      for (size_t i = 0; i < _dict.size(); ++i)
      {
        if (_isWordUsed[i] != _invalidIsWordUsed)
          continue;
        if (wordDistance(currentWord, _dict[i]) <= _maxWordDistance)
          similarWordIndexes.push_back(i);
      }
    });
    return similarWordIndexes;
  }

  void markAsUsed(const std::vector<size_t> & wordIndexes, int waveIndex)
  {
    std::for_each (wordIndexes.begin(), wordIndexes.end(), [&](size_t i) {
      _isWordUsed[i] = waveIndex;});
  }

  bool isResultAchieved(const std::vector<size_t> & wordIndexes) const
  {
    return std::find(wordIndexes.begin(), wordIndexes.end(), _targetWordIndex) != wordIndexes.end();;
  }

  std::vector<std::string> getResult() const
  {
    assert(_isWordUsed[_targetWordIndex] != _invalidIsWordUsed);
    const size_t lastWaveIndex = _isWordUsed[_targetWordIndex];
    std::vector<std::string> result(lastWaveIndex + 1);
    size_t lastWordIndex = _targetWordIndex;
    for (int i = lastWaveIndex; i >= 0; --i)
    {
      result[i] = _dict[lastWordIndex];
      std::string word = _dict[lastWordIndex];
      lastWordIndex = -1;
      for (size_t j = 0; j < _isWordUsed.size(); ++j)
      {
        if (_isWordUsed[j] == i-1 && wordDistance(word, _dict[j]) <= _maxWordDistance)
        {
          lastWordIndex = j;
          break;
        }
      }
    }
    return result;
  }

public:
  WordConverter(std::string sourceWord, std::string targerWord, std::istream &dictStream)
  {
    if (sourceWord.size() != targerWord.size())
      throw std::invalid_argument("Sources and target word sizes should be equal");
    fillDictionary(dictStream, sourceWord.size());
    _targetWordIndex = getWordIndex(targerWord);
    _sourceWordIndex = getWordIndex(sourceWord);
  }

  std::vector<std::string> DoWork()
  {
    //рассматриваем набор слов как граф. слова - узлы. количество различающихся букв между словами - вес ребра.
    //не учитываем ребра вес которых больше 1
    //запускаем подобие Дейкстры
    size_t nMaxIter = 10000;
    _isWordUsed = std::vector<int>(_dict.size(), -1);
    std::vector<size_t> waveWords = std::vector<size_t>{ _sourceWordIndex };

    for (size_t waveIndex = 0; waveIndex < nMaxIter; ++waveIndex)
    {
      markAsUsed(waveWords, waveIndex);
      if (isResultAchieved(waveWords))
        return getResult();

      waveWords = getSimilarWords(waveWords);
      if (waveWords.size() == 0)
        throw std::runtime_error("No sequence between source and target words");
    }
    throw std::runtime_error("Maximum iterations count reached");
  }
};
typedef std::unique_ptr<WordConverter> WordConverterPtr;

class Test
{
private:
  void basic()
  {
    std::string dict = "КОТ\nТОН\nНОТА\nКОТЫ\nРОТ\nРОТА\nТОТ";

    {
      std::stringstream ss;
      ss << dict;
      auto wc = WordConverter("КОТ", "ТОН", ss);
      assert(wc._dict.size() == 4);
      auto res = wc.DoWork();
      assert(res.size() == 3);
      assert(res[0] == "КОТ");
      assert(res[1] == "ТОТ");
      assert(res[2] == "ТОН");
    }
    {
      std::stringstream ss;
      ss << dict;
      auto wc = WordConverter("КОТ", "ТОТ", ss);
      assert(wc._dict.size() == 4);
      auto res = wc.DoWork();
      assert(res.size() == 2);
      assert(res[0] == "КОТ");
      assert(res[1] == "ТОТ");
    }
    {
      std::stringstream ss;
      ss << dict;
      auto wc = WordConverter("КОТ", "КОТ", ss);
      assert(wc._dict.size() == 4);
      auto res = wc.DoWork();
      assert(res.size() == 1);
      assert(res[0] == "КОТ");
    }
  }

  void sameWords()
  {
    std::stringstream ss;
    ss << "КОТ\nТОН\nНОТА\nКОТЫ\nРОТ\nРОТА\nТОТ";
    auto res = WordConverter("КОТ", "КОТ", ss).DoWork();
    assert(res.size() == 1);
  }


  void invalidInput()
  {
    std::string dict = "КОТ\nТОН\nНОТА\nКОТЫ\nРОТ\nРОТА\nТОТ\nТИП";

    int exceptionCount = 0;

    //words not in dict
    try
    {
      std::stringstream ss;
      ss << dict;
      auto res = WordConverter("ЖМОТ", "КРОТ", ss).DoWork();
    }
    catch (const std::exception e)
    {
      ++exceptionCount;
    }

    //no way between words
    try
    {
      std::stringstream ss;
      ss << dict;
      auto res = WordConverter("КОТ", "ТИП", ss).DoWork();
    }
    catch (const std::exception)
    {
      ++exceptionCount;
    }

    try
    {
      std::stringstream ss;
      ss << dict;
      auto res = WordConverter("КОТ", "РОТА", ss).DoWork();
    }
    catch (const std::exception)
    {
      ++exceptionCount;
    }

    assert(exceptionCount == 3);
  }
public:
  void Run()
  {
    basic();
    sameWords();
    invalidInput();
  }
};

class WordConverterBuilder
{
public:
  static WordConverterPtr Build(int argc, char* argv[])
  {
    if (argc < 3)
      throw std::invalid_argument("Usage: app ./path_file_with_words ./path_to_dictionary");
    std::string file1Path = argv[1];
    std::string sourceWord, targetWord;
    {
      std::ifstream if1(file1Path);
      if (!std::getline(if1, sourceWord) || !std::getline(if1, targetWord))
        throw std::invalid_argument("Can't read file");
    }
    file1Path = argv[2];
    std::ifstream ifs(file1Path);
    return std::make_unique<WordConverter>(sourceWord, targetWord, ifs);
  }
};


int main(int argc, char* argv[])
{
  Test().Run();
  WordConverterPtr wc;
  try
  {
    wc = WordConverterBuilder::Build(argc, argv);
  }
  catch (const std::exception &e)
  {
    std::cout << "Invalid input data." << e.what() << std::endl;
    return 1;
  }

  try
  {
    auto res = wc->DoWork();
    std::for_each (res.begin(), res.end(), [&](const std::string & word) 
    {
      std::cout << word << std::endl;
    });
  }
  catch (const std::exception &e)
  {
    std::cout << "Result not found." << e.what() << std::endl;
  }
  return 0;
}
