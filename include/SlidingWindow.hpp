#pragma once
#include "Deque.hpp"
#include "MonotonicDeque.hpp"

template<int W>
class SlidingWindow {
	private:
		MonotonicDeque<W> mono;
		Deque<double, W> win;   // raw values in the current window
		double sum = 0;
		long long count = 0;    // total samples seen

	public:
		void add(double x){
			// Evict oldest BEFORE pushing, so capacity W is enough
			if(win.getSize() == W){
				sum -= win.getFront();
				win.pop_front();
			}
			win.push_back(x);
			sum += x;

			mono.push(x);
			count++;
		}

		double getMax(){ 
			return mono.getMax(); 
		}

		double getMin(){
			return mono.getMin(); 
		}

		double getAverage(){
			if(win.getSize() == 0) return 0;
			return sum / win.getSize();
		}

		bool ready(){
			return count >= W;   // window fully filled
		}
};
