/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef INCLUDE_DATA_CONSTRUCTS_SKIPPEDRELATIVEKEY_H_
#define INCLUDE_DATA_CONSTRUCTS_SKIPPEDRELATIVEKEY_H_

#include "Key.h"
#include "rkey.h"
#include <stdint.h>

#include "../streaming/Streams.h"

#include <stdint.h>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace cclient
{
namespace data
{


/**
 * Based on the accumulo relative key.
 */
class SkippedRelativeKey : public cclient::data::streams::StreamInterface
{
protected:
    Key *prevKey;
    RelativeKey *rkey;
    int skipped;

    void
    fastSkip (cclient::data::streams::InputStream *stream, Key *seekKey, std::vector<char> *valCopy,
              Key *prevKey, Key *currKey)
    {

        std::vector<char> row, cf, cq, cv;
        std::vector<char> prevRow, prevCf, prevCq, prevVis;

        std::pair<char*, size_t> scratch = seekKey->getRow ();
        std::vector<char> stopRow, stopCf, stopCq, stopCv;
        stopRow.insert (stopRow.end (), scratch.first,
                        scratch.first + scratch.second);
        scratch = seekKey->getColFamily ();
        stopCf.insert (stopCf.end (), scratch.first,
                       scratch.first + scratch.second);
        scratch = seekKey->getColQualifier ();
        stopCq.insert (stopCq.end (), scratch.first,
                       scratch.first + scratch.second);

        scratch = seekKey->getColVisibility ();
        stopCv.insert (stopCv.end (), scratch.first,
                       scratch.first + scratch.second);

        uint64_t timestamp = 0;
        uint64_t prevTimestamp = 0;
        bool previousDeleted = false;

        int rowCmp = -1, cfCmp = -1, cqCmp = -1, cvCmp = -1;

        if (NULL != currKey)
        {

            scratch = currKey->getRow ();
            prevRow.insert (prevRow.end (), scratch.first,
                            scratch.first + scratch.second);

            scratch = currKey->getColFamily ();
            prevCf.insert (prevCf.end (), scratch.first,
                           scratch.first + scratch.second);

            scratch = currKey->getColQualifier ();
            prevCq.insert (prevCq.end (), scratch.first,
                           scratch.first + scratch.second);

            scratch = currKey->getColVisibility ();
            prevVis.insert (prevVis.end (), scratch.first,
                            scratch.first + scratch.second);

            prevTimestamp = currKey->getTimeStamp ();

            std::pair<char*, size_t> scratch = currKey->getRow ();
            row.insert (prevRow.end (), scratch.first,
                        scratch.first + scratch.second);

            scratch = currKey->getColFamily ();
            cf.insert (prevCf.end (), scratch.first,
                       scratch.first + scratch.second);

            scratch = currKey->getColQualifier ();
            cq.insert (prevCq.end (), scratch.first,
                       scratch.first + scratch.second);

            scratch = currKey->getColVisibility ();
            cv.insert (prevVis.end (), scratch.first,
                       scratch.first + scratch.second);

            timestamp = currKey->getTimeStamp ();

            if (row >= stopRow)
            {
                if (row > stopRow)
                {
                    rkey = new RelativeKey (new Key (currKey),
                                            new Key (currKey));
                    skipped = 0;
                    this->prevKey = prevKey;

                    return;
                }

                if (cf >= stopCf)
                {
                    if (cf > stopCf)
                    {
                        rkey = new RelativeKey (new Key (currKey),
                                                new Key (currKey));
                        skipped = 0;
                        this->prevKey = prevKey;

                        return;
                    }

                    if (cq > stopCq)
                    {
                        rkey = new RelativeKey (new Key (currKey),
                                                new Key (currKey));
                        skipped = 0;
                        this->prevKey = prevKey;

                        return;
                    }
                }
            }

        }
        else
        {
            char fieldsSame = -1;
            
            uint16_t count = 0;

            Key *newPrevKey = NULL;
            while (true)
            {
                previousDeleted = (fieldsSame & RelativeKey::DELETED)
                                  == RelativeKey::DELETED;

                fieldsSame = stream->readByte ();

                if ((fieldsSame & RelativeKey::PREFIX_COMPRESSION_ENABLED)
                        == RelativeKey::PREFIX_COMPRESSION_ENABLED)
                {
                    stream->readByte ();

                }
                		

                bool changed = false;

                changed = readPrefix (stream, &rowCmp, RelativeKey::ROW_SAME,
                                      RelativeKey::ROW_PREFIX, fieldsSame, &row,
                                      &prevRow, &stopRow);

                if (readPrefix (stream, &cfCmp, RelativeKey::CF_SAME,
                                RelativeKey::CF_PREFIX, fieldsSame, &cf,
                                &prevCf, &stopCf))
                {
                    changed = true;
                }

                if (readPrefix (stream, &cqCmp, RelativeKey::CQ_SAME,
                                RelativeKey::CQ_PREFIX, fieldsSame, &cq,
                                &prevCq, &stopCq))
                {
                    changed = true;
                }

                if (readPrefix (stream, &cvCmp, RelativeKey::CV_SAME,
                                RelativeKey::CV_PREFIX, fieldsSame, &cv,
                                &prevVis, &stopCv))
                {
                    changed = true;
                }

                if ((fieldsSame & RelativeKey::TS_SAME) != RelativeKey::TS_SAME)
                {
                    prevTimestamp = timestamp;

                    timestamp = stream->readEncodedLong();
                    if ((fieldsSame & RelativeKey::TS_DIFF)
                            != RelativeKey::TS_DIFF)
                    {
                        timestamp += prevTimestamp;
                    }
                }

                readValue (stream, valCopy);

                count++;

                if (changed && rowCmp >= 0)
                {
                    if (rowCmp > 0)
                        break;

                    if (cfCmp >= 0)
                    {
                        if (cfCmp > 0)
                            break;

                        if (cqCmp >= 0)
                            break;
                    }
                }
            }

            if (count > 1)
            {
                std::vector<char> *rowPtr, *cfPtr, *cqPtr, *cvPtr;
                rowPtr =
                    (fieldsSame & RelativeKey::ROW_SAME)
                    == RelativeKey::ROW_SAME ? &row : &prevRow;
                cfPtr =
                    (fieldsSame & RelativeKey::CF_SAME)
                    == RelativeKey::CF_SAME ? &cf : &prevCf;
                cqPtr =
                    (fieldsSame & RelativeKey::CQ_SAME)
                    == RelativeKey::CQ_SAME ? &cq : &prevCq;
                cvPtr =
                    (fieldsSame & RelativeKey::CV_SAME)
                    == RelativeKey::CV_SAME ? &cv : &prevVis;
                long returnTs =
                    (fieldsSame & RelativeKey::TS_SAME)
                    == RelativeKey::TS_SAME ? timestamp : prevTimestamp;
                newPrevKey = new Key ();
                newPrevKey->setRow (rowPtr->data (), rowPtr->size ());
                newPrevKey->setColFamily (cfPtr->data (), cfPtr->size ());
                newPrevKey->setColQualifier (cqPtr->data (), cqPtr->size ());
                newPrevKey->setColVisibility (cvPtr->data (), cvPtr->size ());
                newPrevKey->setTimeStamp (returnTs);
                newPrevKey->setDeleted (previousDeleted);

            }
            else if (count == 1)
            {
                if (currKey != NULL)
                {
                    newPrevKey = new Key (prevKey);
                }
                else
                {
                    newPrevKey = new Key (prevKey);
                }
            }
            else
            {
                throw std::runtime_error ("Illegal state");
            }

            prevKey = newPrevKey;
            rkey = new RelativeKey ();
            Key *baseKey = new Key ();
            baseKey->setRow (row.data (), row.size ());
            baseKey->setColFamily (cf.data (), cf.size ());
            baseKey->setColQualifier (cq.data (), cq.size ());
            baseKey->setColVisibility (cv.data (), cv.size ());
            baseKey->setTimeStamp (timestamp);
            baseKey->setDeleted ((fieldsSame & RelativeKey::DELETED) != 0);
            rkey->setBase (baseKey);
            rkey->setPrevious (new Key (baseKey));
            skipped = count;
        }
    }

    bool
    readPrefix (cclient::data::streams::InputStream *stream, int *comparison, uint8_t SAME_FIELD,
                uint8_t PREFIX, char fieldsSame, std::vector<char> *field,
                std::vector<char> *prevField, std::vector<char> *stopField)
    {
        std::vector<char> tmp;

        field->swap (*prevField);


        if ((fieldsSame & SAME_FIELD) != SAME_FIELD)
        {
            if ((fieldsSame & PREFIX) == PREFIX)
            {
                readPrefix (stream, field, prevField);
            }
            else
                read (stream, field);

            if (*field >= *stopField)
            {
                if (*field > *stopField)
                {
                    *comparison = 1;
                }
                else
                    *comparison = 1;
            }
            else
            {
                *comparison = -1;
            }
            return true;
        }
        return false;
    }

    void
    readPrefix (cclient::data::streams::InputStream *stream, std::vector<char> *row, std::vector<char> *prevRow)
    {
        uint32_t prefixLen = stream->readHadoopLong ();
        uint32_t remainingLen = stream->readHadoopLong ();
        row->insert (row->begin (), prevRow->data (),
                     prevRow->data () + prefixLen);
        char *array = new char[remainingLen];
        stream->readBytes (array, remainingLen);
        row->insert (row->end (), array, array + remainingLen);
        delete[] array;
    }

    void
    readValue (cclient::data::streams::InputStream *stream, std::vector<char> *val)
    {
        uint32_t len = stream->readInt ();
        read (stream, val, len);
    }

    void
    read (cclient::data::streams::InputStream *stream, std::vector<char> *row)
    {
        uint32_t len = stream->readHadoopLong ();
        read (stream, row, len);
    }

    void
    read (cclient::data::streams::InputStream *stream, std::vector<char> *input, uint32_t len)
    {
        char *array = new char[len];
        stream->readBytes (array, len);

        input->insert (input->begin (), array, array + len);
        delete[] array;
    }

public:
    SkippedRelativeKey (RelativeKey *rkey, int skipped, Key *prevKey) :
        rkey (rkey), skipped (skipped), prevKey (prevKey)
    {

    }

    SkippedRelativeKey (cclient::data::streams::InputStream *stream, Key *seekKey,
                        std::vector<char> *valCopy, Key *prevKey, Key *currKey) :
        SkippedRelativeKey (NULL, 0, NULL)
    {
        fastSkip (stream, seekKey, valCopy, prevKey, currKey);

    }

    virtual
    ~SkippedRelativeKey ()
    {

        if (NULL != prevKey)
            delete prevKey;
    }

    RelativeKey *
    getRelativeKey ()
    {
        return rkey;
    }
    Key *
    getPrevKey ()
    {
        return prevKey;
    }
    int
    getSkipped ()
    {
        return skipped;
    }

}
;

}
}

#endif /* INCLUDE_DATA_CONSTRUCTS_SKIPPEDRELATIVEKEY_H_ */
