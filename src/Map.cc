/**
 * This file is part of ORB-SLAM2.
 *
 * Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
 * For more information see <https://github.com/raulmur/ORB_SLAM2>
 *
 * ORB-SLAM2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ORB-SLAM2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Map.h"

#include <mutex>
#include <Converter.h>
#include <ostream>
#include <sstream>

namespace ORB_SLAM2
{

    Map::Map() : mnMaxKFid(0), mnBigChangeIdx(0)
    {
    }

    void Map::AddKeyFrame(KeyFrame *pKF)
    {
        unique_lock<mutex> lock(mMutexMap);
        mspKeyFrames.insert(pKF);
        if (pKF->mnId > mnMaxKFid)
            mnMaxKFid = pKF->mnId;
    }

    void Map::AddMapPoint(MapPoint *pMP)
    {
        unique_lock<mutex> lock(mMutexMap);
        mspMapPoints.insert(pMP);
    }

    void Map::EraseMapPoint(MapPoint *pMP)
    {
        unique_lock<mutex> lock(mMutexMap);
        mspMapPoints.erase(pMP);

        // TODO: This only erase the pointer.
        // Delete the MapPoint
    }

    void Map::EraseKeyFrame(KeyFrame *pKF)
    {
        unique_lock<mutex> lock(mMutexMap);
        mspKeyFrames.erase(pKF);

        // TODO: This only erase the pointer.
        // Delete the MapPoint
    }

    void Map::SetReferenceMapPoints(const vector<MapPoint *> &vpMPs)
    {
        unique_lock<mutex> lock(mMutexMap);
        mvpReferenceMapPoints = vpMPs;
    }

    void Map::InformNewBigChange()
    {
        unique_lock<mutex> lock(mMutexMap);
        mnBigChangeIdx++;
    }

    int Map::GetLastBigChangeIdx()
    {
        unique_lock<mutex> lock(mMutexMap);
        return mnBigChangeIdx;
    }

    vector<KeyFrame *> Map::GetAllKeyFrames()
    {
        unique_lock<mutex> lock(mMutexMap);
        return vector<KeyFrame *>(mspKeyFrames.begin(), mspKeyFrames.end());
    }

    vector<MapPoint *> Map::GetAllMapPoints()
    {
        unique_lock<mutex> lock(mMutexMap);
        return vector<MapPoint *>(mspMapPoints.begin(), mspMapPoints.end());
    }

    long unsigned int Map::MapPointsInMap()
    {
        unique_lock<mutex> lock(mMutexMap);
        return mspMapPoints.size();
    }

    long unsigned int Map::KeyFramesInMap()
    {
        unique_lock<mutex> lock(mMutexMap);
        return mspKeyFrames.size();
    }

    vector<MapPoint *> Map::GetReferenceMapPoints()
    {
        unique_lock<mutex> lock(mMutexMap);
        return mvpReferenceMapPoints;
    }

    long unsigned int Map::GetMaxKFid()
    {
        unique_lock<mutex> lock(mMutexMap);
        return mnMaxKFid;
    }

    void Map::clear()
    {
        for (set<MapPoint *>::iterator sit = mspMapPoints.begin(), send = mspMapPoints.end(); sit != send; sit++)
            delete *sit;

        for (set<KeyFrame *>::iterator sit = mspKeyFrames.begin(), send = mspKeyFrames.end(); sit != send; sit++)
            delete *sit;

        mspMapPoints.clear();
        mspKeyFrames.clear();
        mnMaxKFid = 0;
        mvpReferenceMapPoints.clear();
        mvpKeyFrameOrigins.clear();
    }

    void Map::SaveKeyFrame(ofstream &f, KeyFrame *kf)
    {
        KeyId.push_back(kf->mnId);
        // 保存当前关键帧的id
        f << KeyId.end() - KeyId.begin() - 1 << " ";
        // 关键帧内参
        f << kf->fx << " " << kf->fy << " " << kf->cx << " " << kf->cy << " ";
        // 保存当前关键帧的位姿
        cv::Mat Tcw = kf->GetPose();
        cout << "GetPose " << std::to_string(kf->mTimeStamp) << "\nTcw\n"
             << Tcw << endl;
        cv::Mat Rcw = Tcw.rowRange(0, 3).colRange(0, 3);
        cout << "Rcw\n"
             << Rcw << endl;
        // 通过四元数保存旋转矩阵
        std::vector<float> Quat = Converter::toQuaternion(Rcw);

        if (Quat.empty())
            cout << "vector Quat is empty" << endl;
        else
            cout << "vector Quat is not empty" << endl;

        for (int i = 0; i < 4; ++i)
        {
            f << Quat[(3 + i) % 4] << " "; // qw, qx, qy, qz
        }
        // 保存平移
        for (int i = 0; i < 3; ++i)
        {
            f << Tcw.at<float>(i, 3) << " ";
        }
        ostringstream sTimeStamp;
        sTimeStamp << std::to_string(kf->mTimeStamp);
        f << sTimeStamp.str();
        f << "\n";
    }

    void Map::SaveMapPoint(ofstream &f, MapPoint *mp)
    {
        // 保存当前MapPoint世界坐标值
        cv::Mat mpWorldPos = mp->GetWorldPos();
        f << " " << mpWorldPos.at<float>(0) << " " << mpWorldPos.at<float>(1) << " " << mpWorldPos.at<float>(2) << " ";
        f << (mp->nObs) / 2 << " ";

        std::map<KeyFrame *, size_t> mapObservation = mp->GetObservations();
        for (auto mit = mapObservation.begin(); mit != mapObservation.end(); ++mit)
        {
            int Frameid;
            Frameid = mit->first->mnId;
            auto Keyid = find(KeyId.begin(), KeyId.end(), Frameid) - KeyId.begin();
            f << Keyid << " ";
        }
        f << "\n";
    }

    void Map::Save(const string &filename, const cv::MatSize image_size)
    {
        cout << "SFM Saving to " << filename << endl;
        ofstream f;
        f.open(filename.c_str());

        f << "MVS " << image_size[1] << " " << image_size[0] << endl;
        // 输出关键帧的数量
        cout << "The number of KeyFrames: " << mspKeyFrames.size() << endl;

        unsigned long int nKeyFrames = mspKeyFrames.size();
        f << nKeyFrames << endl;
        for (auto kf : mspKeyFrames)
        {
            SaveKeyFrame(f, kf);
        }
        // 输出空间三维点的数目
        cout << "The number of MapPoints: " << mspMapPoints.size();
        unsigned long int nMapPints = mspMapPoints.size();
        f << nMapPints << endl;

        for (auto mp : mspMapPoints)
        {
            SaveMapPoint(f, mp);
        }
        f.close();
    }

} // namespace ORB_SLAM
