{
  vector<double> candidates;
  vector<int> a_candidates;
  vector<int> z_candidates;
  for (size_t a = 0; a < 33; a++) {
    for (size_t z = 0; z < 13; z++) {
      if ( z>=6 && (double)a/z>(32./11.*(0.95)) && (double)a/z<(32./11.*(1.05)) ) {
        candidates.push_back((double)a/(double)z);
        a_candidates.push_back(a);
        z_candidates.push_back(z);
      }
    }
  }
  for (size_t i = 0; i < candidates.size(); i++) {
    cout<<a_candidates.at(i)<<" "<<z_candidates.at(i)<<" "<<candidates.at(i)<<endl;
  }



  std::vector<int> trackIDMultiVtx{3,4};
   std::vector<int> trackIDOneVtx{0,2,3};
   std::vector<int> diff;
   //no need to sort since it's already sorted
   //but you can sort with:
   //std::sort(std::begin(v1), std::end(v1))

   std::set_difference(trackIDMultiVtx.begin(), trackIDMultiVtx.end(), trackIDOneVtx.begin(), trackIDOneVtx.end(),
       std::inserter(diff, diff.begin()));

   for (auto i : trackIDMultiVtx) std::cout << i << ' ';
   std::cout << "minus ";
   for (auto i : trackIDOneVtx) std::cout << i << ' ';
   std::cout << "is: ";

   for (auto i : diff) std::cout << i << ' ';
   std::cout << '\n';

   if(trackIDMultiVtx.size()!=diff.size())cout<<"common number"<<endl;


}
//
// 17 6 2.83333 //unlikely
// 18 6 3 // (d,d14C) unlikely
// 20 7 2.85714 //unlikely
// 21 7 3 //unlikely
// 23 8 2.875 //unlikely
// 24 8 3 //(d,d8Be) unlikely
// 25 9 2.77778 //unlikely
// 26 9 2.88889 //unlikely
// 27 9 3 //unlikely
// 28 10 2.8 //(d,da) CHECK !
// 29 10 2.9 //2p-1n removal unlikely
// 30 10 3 //(d,d2p) unlikely
// 31 11 2.81818 //of interest compete with (d,dp)?
// 32 11 2.90909 //of interest compete with (d,2p)?
