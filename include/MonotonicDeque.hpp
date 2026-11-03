#pragma once

#include "Deque.hpp"

struct Entry{
	int id;
	double val;
};

template<int W>
class MonotonicDeque{
	private:
		Deque<Entry, W> maxQueue;
		Deque<Entry, W> minQueue;
		int next_id=0;

	public:
		MonotonicDeque(){}

		void push(double sample){
			// Discard if out of window range
			if(!maxQueue.empty() && maxQueue.getFront().id + W <= next_id)
				maxQueue.pop_front();

			if(!minQueue.empty() && minQueue.getFront().id + W <= next_id)
				minQueue.pop_front();

			// Max tracking
			// Pop from back until rear is greater than to value
			while(!maxQueue.empty() && maxQueue.getBack().val <= sample)
				maxQueue.pop_back();
			maxQueue.push_back({next_id, sample});

			// Min tracking
			// Pop from back until rear is less than to value
			while(!minQueue.empty() && minQueue.getBack().val >= sample)
				minQueue.pop_back();
			minQueue.push_back({next_id, sample});

			next_id++;
		}

		double getMax(){
			return maxQueue.getFront().val;
		}

		double getMin(){
			return minQueue.getFront().val;
		}
};
