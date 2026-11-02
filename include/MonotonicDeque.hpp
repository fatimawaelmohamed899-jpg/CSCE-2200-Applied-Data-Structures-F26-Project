#pragma once

#include "Queue.hpp"

struct Entry{
	int id;
	double val;
};

template<int W>
class MonotoicDeque{
	private:
		Queue<Entry, W> maxQueue;
		Queue<Entry, W> minQueue;
		int next_id=0;

	public:
		MonotoicDeque<Entry, W>(){}

		void push(double sample){
			// Window Change
			if(!maxQueue.empty() && maxQueue.getFront().id + W <= next_id)
				maxQueue.pop_front();

			if(!minQueue.empty() && minQueue.getFront().id + W <= next_id)
				minQueue.pop_front();

			// Max tracking
			// Pop from back until rear is greater than to value
			while(!maxQueue.empty() && maxQueue.getBack() <= sample)
				maxQueue.pop_back();
			maxQueue.push_back({next_id, sample});

			// Min tracking
			// Pop from back until rear is less than to value
			while(!minQueue.empty() && minQueue.getBack() >= sample)
				minQueue.pop_back();
			minQueue.push_back({next_id, sample});

			next_id++;
		}

		void getMax(){
			return maxQueue.getFront();
		}

		void getMin(){
			return minQueue.getFront();
		}
};
